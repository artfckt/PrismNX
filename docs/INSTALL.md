# Installing PrismNX 0.3.0

Requires a modded Switch with Atmosphere, a working Tesla/Ultrahand menu and
nx-ovlloader. Designed for Switch Lite; a successful build does not certify
compatibility with a particular firmware or console setup.

## Upgrade from SwitchColor 0.1.0 or 0.2.0

1. Close the overlay and back up `switch/.overlays/SwitchColor.ovl`.
2. Extract `PrismNX-0.3.0-overlay-only.zip` and copy its `switch` directory to
   the SD root, replacing the existing file.
3. Reopen the overlay menu and select **PrismNX**.

The legacy filename is intentional. Existing Fizeau configuration, backups
and report paths are retained. No backend update is needed when upgrading
from this project's 0.1.0/0.2.0 installation. Do not leave a duplicate overlay.

## New installation

1. Power off the console before removing its SD card.
2. Back up any existing `atmosphere/contents/0100000000000F12` directory and
   Fizeau configuration files.
3. Extract `PrismNX-0.3.0-SwitchLite.zip`.
4. Copy the **contents of `sd/`** into the SD root, merging directories.
   Do not copy the `sd` directory itself.

Main paths:

```text
switch/.overlays/SwitchColor.ovl
atmosphere/contents/0100000000000F12/exefs.nsp
atmosphere/contents/0100000000000F12/flags/boot2.flag
atmosphere/exefs_patches/nvnflinger_cmu/*.ips
```

The full bundle replaces Fizeau at that title ID with the locally patched
2.8.3 build. Do not install a second Fizeau module under another ID. The bundle
does not replace `ovlmenu.ovl` or your menu settings.

Check these Fizeau configuration paths in order:

```text
switch/Fizeau/config.ini
config/Fizeau/config.ini
```

If either exists, keep it; the first path has priority. Only when neither
exists, copy `optional/config-initial.ini` to `config/Fizeau/config.ini`,
creating its directories if necessary. The template is neutral with four
complete profiles.

Restart into Atmosphere after a new backend installation. Start a game, open
your configured overlay menu and select PrismNX. The menu shortcut belongs
to Tesla/Ultrahand; PrismNX does not change it.

## Use

Open **Display and presets**. Enable **Color correction**, then select a
slider or preset. Use Left/Right or touch; B returns to the previous page.
**Save for reboot** writes configuration explicitly. Live changes remain
active after closing the overlay, but unsaved changes are lost on reboot.

**Restore opening state** restores only the live state captured when the
overlay opened. Save again if you want that restored state to persist.
The first `config.ini.switchcolor.bak` is retained across later saves.
See [TOOLKIT.md](TOOLKIT.md) for presets, telemetry and quick controls.

## Troubleshooting and recovery

- **Fizeau service is not running:** check the module and `boot2.flag`, then
  restart into Atmosphere and use Retry connection.
- **Invalid configuration:** back up your current file and check its syntax
  and completeness. Unknown global keys are rejected; profile comments and
  unknown profile keys are preserved.
- **Uncertain state:** use Recover after error. If recovery fails, restart
  the console. Reading stored values alone cannot verify hardware recovery.
- **SD error:** inspect free space, permissions and the paths in the error.
  `.switchcolor.tmp`, `.switchcolor.rollback` and `.switchcolor.bak.tmp`
  can indicate an interrupted operation. Keep copies and recover from a
  verified backup rather than deleting them blindly.
- **No visible effect:** enable correction, apply Vibrant and compare with
  correction disabled. Exact CMU patch compatibility requires a console test.
- **Unavailable information:** press A on a metric for its full error code.
  Services or readings may be unavailable on a particular setup.

For rollback, close the overlay and restore its previous file. For backend
rollback, power off and restore the previous Fizeau module/configuration.
If installed exclusively for PrismNX, temporarily rename its `boot2.flag`
to disable startup. Never delete the general `atmosphere` or `switch` folders.
