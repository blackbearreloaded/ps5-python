#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
app_dir="${1:-apps/hello}"

PS5_APP_DIR="$app_dir" bash "$root_dir/tools/run_ps5.sh" --app
