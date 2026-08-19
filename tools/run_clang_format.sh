#!/usr/bin/env bash
set -euo pipefail

root_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
formatter=${CLANG_FORMAT:-clang-format}

if ! command -v "$formatter" >/dev/null 2>&1; then
    echo "clang-format is required" >&2
    exit 1
fi

mapfile -d '' sources < <(find "$root_dir/src" -type f \( -name '*.c' -o -name '*.h' \) -print0)
if [[ "${1:-}" == "--check" ]]; then
    "$formatter" --dry-run --Werror "${sources[@]}"
else
    "$formatter" -i "${sources[@]}"
fi
