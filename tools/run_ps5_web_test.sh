#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
export PS5_WEB_ELF="${PS5_WEB_ELF:-$root_dir/build/ps5/python-web-test.elf}"
export PS5_WEB_PORT="${PS5_WEB_PORT:-9601}"

exec bash "$root_dir/tools/run_ps5_web.sh" "$@"
