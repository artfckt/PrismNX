"""Prepare an isolated, patched Fizeau source tree; never modify the submodule."""
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
source = ROOT / "third_party/fizeau"
target = ROOT / "build/fizeau-backend"
target.mkdir(parents=True, exist_ok=True)
for name in ("common", "sysmodule", "lib/inih"):
    shutil.copytree(source / name, target / name, dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns(".git", "build", "out", "*.a"))
subprocess.run(["git", "apply", "--directory=build/fizeau-backend",
                "patches/fizeau-config-eof.patch"], cwd=ROOT, check=True)
print("Prepared Fizeau 2.8.3 + final-profile loading fix")
