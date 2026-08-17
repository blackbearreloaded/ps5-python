$ErrorActionPreference = 'Stop'

$version = '3.14.7'
$commit = '823f0323ee6ec1402088b73bce1a38473cac36dc'
$root = Split-Path -Parent $PSScriptRoot
$source = Join-Path $root 'upstream/cpython'
$patch = Join-Path $root 'patches/ps5-freebsd-configure.patch'

if (-not (Test-Path -LiteralPath (Join-Path $source '.git'))) {
    New-Item -ItemType Directory -Path (Split-Path -Parent $source) -Force | Out-Null
    git -c core.autocrlf=false clone --depth 1 --branch "v$version" https://github.com/python/cpython.git $source
}

$actual = (git -C $source rev-parse HEAD).Trim()
if ($actual -ne $commit) {
    throw "Unexpected CPython source commit: $actual; expected $commit"
}

$oldErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = 'Continue'
$check = git -C $source apply --check $patch 2>&1
$checkExit = $LASTEXITCODE
$ErrorActionPreference = $oldErrorActionPreference
if ($checkExit -eq 0) {
    git -C $source apply $patch
} else {
    $oldErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $reverse = git -C $source apply --reverse --check $patch 2>&1
    $reverseExit = $LASTEXITCODE
    $ErrorActionPreference = $oldErrorActionPreference
    if ($reverseExit -ne 0) {
        throw "CPython source does not match the expected unpatched or patched state. Recreate upstream/cpython and run make source-fetch again. Details: $check"
    }
}

Write-Output "CPython $version is available at $source"
