#!/usr/bin/env bash
set -euo pipefail

root_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
tidy=${CLANG_TIDY:-clang-tidy}

if ! command -v "$tidy" >/dev/null 2>&1; then
    echo "clang-tidy is required" >&2
    exit 1
fi

mapfile -d '' sources < <(find "$root_dir/src" -type f -name '*.c' -print0)
"$tidy" "${sources[@]}" --quiet --warnings-as-errors='*' -- \
    -std=c11 \
    -DCPYTHON_PS5 \
    -D_POSIX_C_SOURCE=200809L \
    -I"$root_dir/include" \
    -I"$root_dir/platform" \
    -I"$root_dir/upstream/cpython/Include" \
    -I"$root_dir/upstream/cpython" \
    -I"$root_dir/upstream/cpython/Include/internal" \
    -I"$root_dir/build/ps5" \
    -I"$root_dir/build/ps5/deps/user/homebrew/include"
