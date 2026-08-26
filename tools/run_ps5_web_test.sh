#!/usr/bin/env bash
# PS5-Python - CPython for the PlayStation 5.
# Copyright (C) 2026 BlackBearReloaded
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
export PS5_WEB_ELF="${PS5_WEB_ELF:-$root_dir/build/ps5/python-web-test.elf}"
export PS5_WEB_PORT="${PS5_WEB_PORT:-9601}"

exec bash "$root_dir/tools/run_ps5_web.sh" "$@"
