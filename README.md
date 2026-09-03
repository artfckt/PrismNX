# PrismNX

**Color your games. Know your console.**

PrismNX is a native Nintendo Switch overlay with live color controls, 18 visual
presets, console telemetry and quick brightness/audio controls. Designed for
Switch Lite running Atmosphere, it opens through Tesla or Ultrahand and uses
Fizeau for color processing. Previously named SwitchColor.

[Download the latest release](https://github.com/DannyDulgheru/PrismNX/releases/latest)
| [Installation](docs/INSTALL.md) | [User guide](docs/TOOLKIT.md)

## Features

- Saturation, contrast, gamma, color temperature, hue and luminance sliders.
- 18 presets: natural/OLED styles, contrast, warm/cinema/evening and creative.
- Seven information pages covering battery, temperatures, CPU/GPU/EMC clocks,
  network, firmware, active Title ID and SD storage.
- Quick backlight and active-output volume controls, with readback verification.
- Local diagnostic reports and shortcuts to installed Status Monitor,
  FPSLocker, sys-clk and Sysmodules overlays.
- Explicit save, opening-state restoration, retained backups and recovery
  after uncertain color operations.

**Version 0.3.0:** English interface, preset descriptions, errors and guides.
The ARM64 build and host regression tests pass. Console runtime behavior has
not yet been confirmed; see the [validation record](docs/VALIDATION.md).

OLED presets are aesthetic LCD color adjustments, not OLED panel emulation.
Spatial sharpening and automatic per-game profiles are not implemented.
Clock readings are not CPU/GPU utilization or FPS.

## Install or upgrade

For a new installation, use `PrismNX-0.3.0-SwitchLite.zip` and follow
[INSTALL.md](docs/INSTALL.md). Existing SwitchColor users can use
`PrismNX-0.3.0-overlay-only.zip`.

The filename remains **`switch/.overlays/SwitchColor.ovl`** for in-place upgrades;
the overlay displays **PrismNX**. Existing Fizeau settings, backup suffixes and
`config/SwitchColor/reports/` are retained. Do not install a second copy under
another filename. The release does not include Tesla/Ultrahand or nx-ovlloader.

The full package contains Fizeau 2.8.3 with a documented local fix for reading
the final profile, plus the official CMU patches. It does not contain an
active replacement Fizeau configuration; a neutral template is optional.

## Build

Requires devkitPro, devkitA64, libnx, switch-glm, Python 3 and MSYS2 on Windows.
Host tests use the MSYS2 `gcc` package. Tested SDK versions are in the manifest.

```powershell
git clone https://github.com/DannyDulgheru/PrismNX.git
cd PrismNX
git submodule update --init third_party/fizeau
git -C third_party/fizeau submodule update --init lib/libtesla lib/inih/inih
.\scripts\build.ps1 -DevkitPro 'C:\devkitPro'
.\scripts\build.ps1 -DevkitPro 'C:\devkitPro' -Test
.\scripts\build-backend.ps1 -DevkitPro 'C:\devkitPro'
python tests/test_backend_boot.py --bash 'C:\devkitPro\msys2\usr\bin\bash.exe'
python scripts/package.py
```

Adjust the SDK path to your installation. For the complete source release,
skip the clone/submodule steps: the used dependency sources are included.
Additional MSYS2 packages: `pacman -S --needed gcc switch-glm`.

Linux/devkitPro: `make`, `make test`, `python3 scripts/prepare_backend.py`,
then `make -C build/fizeau-backend/sysmodule` and `python3 scripts/package.py`.
Python must be accessible from the shell running make.

`main.cpp` provides the hub; `*_ui.cpp` provide pages; `telemetry.cpp` reads
console services. `presets.cpp` defines the catalog; `backend.cpp` verifies
color transactions; `switch_backend.cpp` handles IPC; `storage.cpp` saves
configuration. See [architecture](docs/design.md) for details.

## License and credits

GPL-2.0-or-later. Original dependency notices are retained.
Thanks to averne/Fizeau, WerWolv/libtesla, switchbrew/libnx, benhoyt/inih and GLM.
See [THIRD_PARTY.md](THIRD_PARTY.md) for revisions and local patches.
