# PS5-Python - CPython for the PlayStation 5.
# Copyright (C) 2026 BlackBearReloaded
# SPDX-License-Identifier: GPL-3.0-or-later

"""PS5 adaptation of CPython Lib/test/test_time.py."""

import time


wall_before = time.time()
mono_before = time.monotonic()
perf_before = time.perf_counter()
assert wall_before > 0
assert time.time() >= wall_before
assert time.monotonic() >= mono_before
assert time.perf_counter() >= perf_before
assert time.time_ns() > 0
assert time.monotonic_ns() > 0
assert time.perf_counter_ns() > 0

print("test_time: PASS")
