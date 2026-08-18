"""PS5-safe process management coverage from CPython test_os."""

import os
import sys


if not hasattr(os, "fork") or not sys.platform.startswith(("freebsd", "linux", "darwin")):
    print("test_process: SKIP (fork unavailable on host)")
    raise SystemExit(0)


pid = os.fork()
if pid == 0:
    os._exit(0)

child_pid, status = os.waitpid(pid, 0)
assert child_pid == pid
assert status == 0

print("test_process: fork/waitpid PASS")
