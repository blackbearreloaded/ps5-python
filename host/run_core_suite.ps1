# PS5-Python - CPython for the PlayStation 5.
# Copyright (C) 2026 BlackBearReloaded
# SPDX-License-Identifier: GPL-3.0-or-later

$ErrorActionPreference = 'Stop'

$python = Get-Command python -ErrorAction SilentlyContinue
if ($null -eq $python) {
    $python = Get-Command py -ErrorAction SilentlyContinue
}
if ($null -eq $python) {
    throw 'A host Python interpreter is required.'
}

$root = Split-Path -Parent $PSScriptRoot
$tests = @(Get-ChildItem -LiteralPath (Join-Path $root 'tests') -Recurse -File -Filter '*.py' |
    Where-Object {
        $_.Name -ne 'core_suite.py' -and
        $_.FullName -notlike "*\tests\lifetime\*"
    } |
    Sort-Object FullName)

if ($tests.Count -eq 0) {
    throw 'No language-core test scripts were found.'
}

foreach ($test in $tests) {
    Write-Host ("RUN {0}" -f $test.FullName.Substring($root.Length + 1))
    & $python.Source -S $test.FullName
    if ($LASTEXITCODE -ne 0) {
        throw ("FAILED {0}" -f $test.FullName)
    }
}

Write-Host ("CPYTHON_CORE_SUITE: PASS ({0} scripts)" -f $tests.Count)
