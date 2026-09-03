"""Upgrade only SwitchColor.ovl on the user's ftpsrv SD mount, with backup/readback."""
from argparse import ArgumentParser
from datetime import datetime, timezone
from ftplib import FTP, all_errors
from io import BytesIO
from pathlib import Path
import hashlib
import json
import zipfile

ROOT = Path(__file__).resolve().parents[1]
TARGET = "/sdmc:/switch/.overlays/SwitchColor.ovl"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("--host", required=True)
    parser.add_argument("--port", type=int, default=5000)
    parser.add_argument("--package", type=Path, default=ROOT / "dist/SwitchColor-0.2.0-SwitchLite.zip")
    args = parser.parse_args()
    with zipfile.ZipFile(args.package) as archive:
        manifest = json.loads(archive.read("manifest.json"))
        data = archive.read("sd/switch/.overlays/SwitchColor.ovl")
    if data[0x10:0x14] != b"NRO0" or sha(data) != manifest["sha256"]["switch/.overlays/SwitchColor.ovl"]:
        raise RuntimeError("Invalid overlay or package hash")
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    folder = ROOT / "build/ftp-upgrades" / stamp
    backup = "/sdmc:/switchcolor-backups/" + stamp
    stage = TARGET + ".upload-" + stamp
    receipt = {"version": manifest["version"], "target": TARGET, "sha256": sha(data), "complete": False}

    with FTP() as ftp:
        ftp.connect(args.host, args.port, timeout=10)
        ftp.login()
        ftp.voidcmd("TYPE I")

        def read(path):
            result = BytesIO()
            ftp.retrbinary("RETR " + path, result.write)
            return result.getvalue()

        def upload(path, content):
            ftp.storbinary("STOR " + path, BytesIO(content))
            if read(path) != content:
                raise RuntimeError("Remote readback mismatch: " + path)

        def directory(path):
            parent, name = path.rsplit("/", 1)
            if parent != "/sdmc:" and not parent.startswith("/sdmc:/switchcolor-backups"):
                raise RuntimeError("Unexpected backup parent")
            rows = []
            ftp.retrlines("LIST " + parent, rows.append)
            matches = [r for r in rows if len(r.split(maxsplit=8)) == 9 and r.split(maxsplit=8)[8] == name]
            if matches:
                if not matches[0].startswith("d"):
                    raise RuntimeError("Backup path exists but is not a directory")
            else:
                ftp.mkd(path)

        def record():
            (folder / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")

        old = read(TARGET)  # Upgrade requires an existing installation.
        if old == data:
            print("Already installed; full remote readback matches the package.")
            return
        folder.mkdir(parents=True)
        (folder / "SwitchColor.previous.ovl").write_bytes(old)
        if (folder / "SwitchColor.previous.ovl").read_bytes() != old:
            raise RuntimeError("Local backup verification failed")
        receipt.update({"backup_remote": backup, "previous_sha256": sha(old), "phase": "local backup"})
        record()
        directory("/sdmc:/switchcolor-backups")
        directory(backup)
        upload(backup + "/SwitchColor.ovl", old)
        upload(stage, data)
        receipt["phase"] = "backup and staged upload verified"
        record()
        if read(TARGET) != old:
            raise RuntimeError("Target changed during upload; backup and staged file retained")
        moved = backup + "/SwitchColor.original.ovl"
        ftp.rename(TARGET, moved)
        receipt["phase"] = "original moved; replacement pending"
        record()
        try:
            ftp.rename(stage, TARGET)
        except all_errors:
            ftp.rename(moved, TARGET)
            receipt["phase"] = "original restored after rename failure"
            record()
            raise
        if read(TARGET) != data:
            raise RuntimeError("Final verification failed; retain remote backup for recovery")
        receipt.update({"complete": True, "phase": "verified", "size": len(data)})
        record()
        print("VERIFIED", TARGET, len(data), "bytes")
        print("BACKUP", backup)
        print("RECEIPT", folder / "receipt.json")


if __name__ == "__main__":
    main()
