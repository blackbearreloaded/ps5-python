#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
runs="${PS5_LIFETIME_RUNS:-3}"

case "$runs" in
    ''|*[!0-9]*|0)
        echo "PS5_LIFETIME_RUNS must be a positive integer." >&2
        exit 2
        ;;
esac

for round in $(seq 1 "$runs"); do
    echo "Lifetime run $round/$runs: repeated allocation"
    bash "$root_dir/tools/run_ps5.sh" tests/lifetime/repeated_state.py
    echo "Lifetime run $round/$runs: recursion and errors"
    bash "$root_dir/tools/run_ps5.sh" tests/lifetime/recursion_and_errors.py
done

echo "CPYTHON_PS5_LIFETIME: PASS ($runs process runs)"
