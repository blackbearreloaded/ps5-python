$ErrorActionPreference = 'Stop'

$python = Get-Command python -ErrorAction SilentlyContinue
if ($null -eq $python) {
    $python = Get-Command py -ErrorAction SilentlyContinue
}
if ($null -eq $python) {
    throw 'A host Python interpreter is required.'
}

$root = Split-Path -Parent $PSScriptRoot
$script = Join-Path $root 'tests/core_basics.py'

& $python.Source -S $script
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
