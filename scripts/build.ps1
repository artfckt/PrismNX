param(
    [string]$DevkitPro = 'E:\Code\devkitPro',
    [switch]$Test
)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskBash = Join-Path $DevkitPro 'msys2\usr\bin\bash.exe'
if (-not (Test-Path -LiteralPath $taskBash)) {
    throw 'MSYS2 bash lipseste. Specifica -DevkitPro calea catre devkitPro.'
}
# Pass user paths as argv. They are never interpolated into shell source.
$taskShellCode = 'set -e; cd "$(cygpath -u "$1")"; export DEVKITPRO="$(cygpath -u "$2")"; unset MSYS2_ARG_CONV_EXCL; if [ "$3" = test ]; then make test; else make -j4; fi'
$taskMode = if ($Test) { 'test' } else { 'build' }
& $taskBash -lc $taskShellCode 'switchcolor-build' $taskRoot $DevkitPro $taskMode
if ($LASTEXITCODE -ne 0) { throw "Build esuat: $LASTEXITCODE" }
