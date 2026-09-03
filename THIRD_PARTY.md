# Sources, licenses and local modifications

SwitchColor source is GPL-2.0-or-later. See LICENSE.

| Dependency | Revision | License | Use |
|---|---|---|---|
| averne/Fizeau | v2.8.3, 5bf3f0d (full revision in submodule and build manifest) | GPL-2.0-or-later | IPC declarations/client; separate display sysmodule |
| WerWolv/libtesla | b32acbca64c78bf37bc456bd386cd6b7148842c8 | GPL-2.0-or-later; see bundled files for stb license | Native overlay UI |
| benhoyt/inih | 4f251f0ff766052c342823dfa52a04f486cc4f94 | BSD-3-Clause | Fizeau configuration parser |
| switchbrew/libnx | Installed devkitPro package; recorded in build manifest | ISC | Switch homebrew SDK |
| GLM (switch-glm) | 0.9.9.7-2 package | MIT | Fizeau color math |

Original copyright notices and license files are retained. Source archives
include the used dependency sources, the local backend patch,
and source archives for the libnx/GLM versions used in this build.
The toolchain itself is supplied by devkitPro, not authored by this project.

## Fizeau patch

`patches/fizeau-config-eof.patch` commits the final successfully parsed profile
to sysmodule state after INI parsing. The original callback only commits a
profile when the next section starts. This patch does not change title ID,
IPC ABI, display register handling or firmware patch selection.

The patched sysmodule is a local derivative, not an official Fizeau release.
Original sources: https://github.com/averne/Fizeau/tree/v2.8.3

## libtesla guards

`scripts/prepare_tesla.py` creates a build-local copy with two small guards:
check stack emptiness before reading its first GUI, and return after B pops a
GUI so subsequent touch handling cannot use its former reference. Our own GUI
buttons also defer pop operations until the update boundary.

Original source: https://github.com/WerWolv/libtesla/tree/b32acbca64c78bf37bc456bd386cd6b7148842c8

## CMU patch files

Copied unchanged from the official release asset:
https://github.com/averne/Fizeau/releases/download/v2.8.3/Fizeau-2.8.3-5bf3f0d.zip

SHA256: `8c44d2cfee17c9020a1f5ba9223e260c96ea716bf016c7cd0371de0685835b85`

The patch filenames identify firmware build IDs. Their presence does not
constitute testing on the user's reported firmware.
