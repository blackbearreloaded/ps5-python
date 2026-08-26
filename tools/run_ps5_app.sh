#!/usr/bin/env bash
# PS5-Python - CPython for the PlayStation 5.
# Copyright (C) 2026 BlackBearReloaded
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
app_dir="${1:-apps/flask_dashboard}"

PS5_APP_DIR="$app_dir" bash "$root_dir/tools/run_ps5.sh" --app
