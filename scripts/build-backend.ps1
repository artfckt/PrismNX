param([string]$DevkitPro = 'C:\devkitPro')
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskBash = Join-Path $DevkitPro 'msys2\usr\bin\bash.exe'
Push-Location $taskRoot
try {
    python scripts/prepare_backend.py
    if ($LASTEXITCODE -ne 0) { throw 'Fizeau source preparation failed.' }
    $taskCode = 'set -e; cd "$(cygpath -u "$1")/build/fizeau-backend"; export DEVKITPRO="$(cygpath -u "$2")"; unset MSYS2_ARG_CONV_EXCL; make -C sysmodule -j4'
    & $taskBash -lc $taskCode 'switchcolor-backend' $taskRoot $DevkitPro
    if ($LASTEXITCODE -ne 0) { throw 'Fizeau module build failed.' }
} finally { Pop-Location }
