param(
    [string]$AppPath = 'apps/hello'
)

$ErrorActionPreference = 'Stop'

$python = Get-Command python -ErrorAction SilentlyContinue
if ($null -eq $python) {
    $python = Get-Command py -ErrorAction SilentlyContinue
}
if ($null -eq $python) {
    throw 'A host Python interpreter is required.'
}

$root = Split-Path -Parent $PSScriptRoot
$app = Join-Path $root $AppPath
$entry = Join-Path $app 'main.py'
$library = Join-Path $app 'lib'

if (-not (Test-Path -LiteralPath (Join-Path $app 'app.json'))) {
    throw "Missing app manifest: $app/app.json"
}
if (-not (Test-Path -LiteralPath $entry)) {
    throw "Missing app entry: $entry"
}

$previousPythonPath = $env:PYTHONPATH
$env:PYTHONPATH = $library
try {
    & $python.Source -S $entry
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
finally {
    $env:PYTHONPATH = $previousPythonPath
}

Write-Host "CPYTHON_APP_HOST: PASS ($AppPath)"
