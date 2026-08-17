$ErrorActionPreference = 'Stop'

$python = Get-Command python -ErrorAction SilentlyContinue
if ($null -eq $python) {
    $python = Get-Command py -ErrorAction SilentlyContinue
}
if ($null -eq $python) {
    throw 'A host Python interpreter is required.'
}

$root = Split-Path -Parent $PSScriptRoot
$successTests = @(
    (Join-Path $root 'tests/lifetime/repeated_state.py'),
    (Join-Path $root 'tests/lifetime/recursion_and_errors.py')
)

$runs = 3
for ($round = 1; $round -le $runs; $round++) {
    foreach ($script in $successTests) {
        Write-Host ("RUN {0}/{1}: {2}" -f $round, $runs, (Resolve-Path -Relative $script))
        & $python.Source -S $script
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }
}

$failureTests = @(
    (Join-Path $root 'tests/lifetime/expected_uncaught.py'),
    (Join-Path $root 'tests/lifetime/expected_syntax.py')
)
foreach ($script in $failureTests) {
    Write-Host ("EXPECT FAILURE: {0}" -f (Resolve-Path -Relative $script))
    $previousErrorAction = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $python.Source -S $script 1>$null 2>$null
    $failureExitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousErrorAction
    if ($failureExitCode -eq 0) {
        throw "Expected failure did not fail: $script"
    }
}

Write-Host ("CPYTHON_LIFETIME_SUITE: PASS ({0} successful scripts x {1} runs; {2} expected failures)" -f $successTests.Count, $runs, $failureTests.Count)
