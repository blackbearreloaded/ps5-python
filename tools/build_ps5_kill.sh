#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk_dir="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"
output="$root_dir/build/ps5/ps5-kill.elf"

if [ ! -f "$sdk_dir/toolchain/prospero.sh" ]; then
    echo "Missing PS5 SDK: $sdk_dir" >&2
    exit 1
fi

source "$sdk_dir/toolchain/prospero.sh"
mkdir -p "$(dirname "$output")"
"$sdk_dir/bin/prospero-clang" -DCPYTHON_PS5 \
    -o "$output" "$root_dir/src/ps5_kill.c"
echo "Built $output"
