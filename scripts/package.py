"""Create reviewable SD/source packages; never write to a connected console."""
from pathlib import Path
import hashlib
import json
import struct
import subprocess
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist"
DOWNLOADS = ROOT / "build/downloads"
VERSION = "0.2.0"
ASSET = "Fizeau-2.8.3-5bf3f0d.zip"
FIZEAU_URL = f"https://github.com/averne/Fizeau/releases/download/v2.8.3/{ASSET}"
FIZEAU_HASH = "8c44d2cfee17c9020a1f5ba9223e260c96ea716bf016c7cd0371de0685835b85"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def fetch(url, name, expected=None):
    path = DOWNLOADS / name
    if not path.exists():
        with urllib.request.urlopen(url, timeout=60) as response:
            data = response.read()
        if expected and digest(data) != expected:
            raise RuntimeError(f"Unexpected download hash: {name}")
        path.write_bytes(data)
    data = path.read_bytes()
    if expected and digest(data) != expected:
        raise RuntimeError(f"Cached download hash mismatch: {name}")
    return path


def revision(path, fallback):
    result = subprocess.run(["git", "-C", str(path), "rev-parse", "HEAD"],
                            capture_output=True, text=True)
    return result.stdout.strip() if result.returncode == 0 else fallback


def neutral_config():
    text = "; SwitchColor: initial neutral configuration; use only if no config exists.\n"
    text += "active = true\nhandheld_profile = profile1\ndocked_profile = profile2\n\n"
    settings = {"temperature": "6500", "saturation": "1.0", "hue": "0.0",
                "contrast": "1.0", "gamma": "2.4", "luminance": "0.0", "range": "0.0-1.0"}
    for i in range(1, 5):
        text += f"[profile{i}]\n"
        for event in ("dusk_begin", "dusk_end", "dawn_begin", "dawn_end"):
            text += f"{event} = 00:00\n"
        for period in ("day", "night"):
            for key, value in settings.items():
                text += f"{key}_{period} = {value}\n"
        text += "components = all\nfilter = none\ndimming_timeout = 00:00\n\n"
    return text.encode()


def zip_bytes(path, files):
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(files.items()):
            archive.writestr(name, data)
    with zipfile.ZipFile(path) as archive:
        if archive.testzip() is not None:
            raise RuntimeError(f"Invalid ZIP: {path}")


def main():
    DIST.mkdir(exist_ok=True)
    DOWNLOADS.mkdir(parents=True, exist_ok=True)
    ovl = (ROOT / "out/SwitchColor.ovl").read_bytes()
    if ovl[0x10:0x14] != b"NRO0":
        raise RuntimeError("Overlay lacks NRO0 header")
    nro_size = struct.unpack_from("<I", ovl, 0x18)[0]
    if ovl[nro_size:nro_size + 4] != b"ASET":
        raise RuntimeError("Overlay lacks metadata asset header")
    elf = (ROOT / "out/SwitchColor.elf").read_bytes()
    if elf[:4] != b"\x7fELF" or struct.unpack_from("<H", elf, 18)[0] != 183:
        raise RuntimeError("Build is not an ARM64 ELF")
    backend = (ROOT / "build/fizeau-backend/sysmodule/out/Fizeau.nsp").read_bytes()
    if backend[:4] != b"PFS0":
        raise RuntimeError("Backend lacks PFS0 header")
    upstream = fetch(FIZEAU_URL, ASSET, FIZEAU_HASH)
    files = {"switch/.overlays/SwitchColor.ovl": ovl,
             "atmosphere/contents/0100000000000F12/exefs.nsp": backend,
             "atmosphere/contents/0100000000000F12/flags/boot2.flag": b"",
             "atmosphere/contents/0100000000000F12/toolbox.json":
                 (ROOT / "third_party/fizeau/sysmodule/toolbox.json").read_bytes()}
    with zipfile.ZipFile(upstream) as archive:
        patches = [n for n in archive.namelist()
                   if n.startswith("atmosphere/exefs_patches/nvnflinger_cmu/") and n.endswith(".ips")]
        if not patches:
            raise RuntimeError("Official release does not contain CMU patches")
        for name in patches:
            data = archive.read(name)
            if not data.startswith(b"PATCH"):
                raise RuntimeError(f"Invalid IPS file: {name}")
            files[name] = data
    manifest = {"project": "SwitchColor", "version": VERSION,
                "target_reported": {"model": "Switch Lite", "atmosphere": "1.11.2|S", "firmware": "20.5.0"},
                "console_tested": False,
                "toolchain": {"devkitA64": "r30-1", "libnx": "4.12.0-1", "switch-glm": "0.9.9.7-2"},
                "fizeau": {"version": "2.8.3 + local EOF fix", "source_revision":
                    revision(ROOT / "third_party/fizeau", "v2.8.3"), "official_asset_sha256": FIZEAU_HASH},
                "libtesla_revision": "b32acbca64c78bf37bc456bd386cd6b7148842c8",
                "sha256": {name: digest(data) for name, data in sorted(files.items())}}
    manifest_bytes = (json.dumps(manifest, indent=2) + "\n").encode()
    (DIST / "manifest.json").write_bytes(manifest_bytes)
    bundle = {"sd/" + name: data for name, data in files.items()}
    bundle["optional/config-initiala.ini"] = neutral_config()
    bundle["INSTALL_RO.md"] = (ROOT / "docs/INSTALL_RO.md").read_bytes()
    bundle["TOOLKIT_RO.md"] = (ROOT / "docs/TOOLKIT_RO.md").read_bytes()
    bundle["manifest.json"] = manifest_bytes
    for name in ("LICENSE", "THIRD_PARTY.md"):
        bundle[name] = (ROOT / name).read_bytes()
    bundle["licenses/Fizeau-LICENSE"] = (ROOT / "third_party/fizeau/LICENSE").read_bytes()
    bundle["licenses/libtesla-LICENSE"] = (ROOT / "third_party/fizeau/lib/libtesla/LICENSE").read_bytes()
    bundle["licenses/inih-LICENSE.txt"] = (ROOT / "third_party/fizeau/lib/inih/inih/LICENSE.txt").read_bytes()
    zip_bytes(DIST / f"SwitchColor-{VERSION}-SwitchLite.zip", bundle)
    zip_bytes(DIST / f"SwitchColor-{VERSION}-overlay-only.zip",
              {"switch/.overlays/SwitchColor.ovl": ovl, "LICENSE": (ROOT / "LICENSE").read_bytes(),
               "TOOLKIT_RO.md": (ROOT / "docs/TOOLKIT_RO.md").read_bytes()})
    (DIST / "SwitchColor.ovl").write_bytes(ovl)

    source_files = {}
    for name in ("source", "include", "scripts", "tests", "patches", "docs", ".agents"):
        for path in (ROOT / name).rglob("*"):
            if path.is_file() and "__pycache__" not in path.parts:
                source_files[path.relative_to(ROOT).as_posix()] = path.read_bytes()
    for name in ("Makefile", "README.md", "LICENSE", "THIRD_PARTY.md", ".gitignore", ".gitmodules"):
        source_files[name] = (ROOT / name).read_bytes()
    # Include only dependency components used in these binaries, with licenses.
    for name in ("common", "sysmodule", "lib/inih", "lib/libtesla"):
        for path in (ROOT / "third_party/fizeau" / name).rglob("*"):
            if path.is_file() and not any(p in (".git", "build", "out", "__pycache__") for p in path.relative_to(ROOT / "third_party/fizeau" / name).parts):
                source_files[path.relative_to(ROOT).as_posix()] = path.read_bytes()
    for name in ("LICENSE", "README.md", "Makefile", ".gitmodules"):
        source_files["third_party/fizeau/" + name] = (ROOT / "third_party/fizeau" / name).read_bytes()
    # Source for the permissively licensed SDK/math components as well.
    for name, url in (
        ("libnx-4.12.0.zip", "https://codeload.github.com/switchbrew/libnx/zip/refs/tags/v4.12.0"),
        ("glm-0.9.9.7.zip", "https://codeload.github.com/g-truc/glm/zip/refs/tags/0.9.9.7"),
    ):
        path = fetch(url, name)
        with zipfile.ZipFile(path) as archive:
            if archive.testzip() is not None: raise RuntimeError(f"Invalid source archive: {name}")
        source_files["third_party/source-archives/" + name] = path.read_bytes()
    source_files["BUILD_MANIFEST.json"] = manifest_bytes
    zip_bytes(DIST / f"SwitchColor-{VERSION}-source.zip", source_files)
    checksums = []
    for path in sorted(DIST.glob("*.zip")):
        checksums.append(f"{digest(path.read_bytes())}  {path.name}")
        print(f"{path.name}: {path.stat().st_size:,} bytes")
    checksums.append(f"{digest(ovl)}  SwitchColor.ovl")
    (DIST / "SHA256SUMS.txt").write_text("\n".join(checksums) + "\n", encoding="utf-8")
    print("Verified ARM64 ELF, NRO/OVL, metadata, PFS0, official IPS hashes and ZIP integrity")


if __name__ == "__main__":
    main()
