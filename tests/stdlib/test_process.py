"""PS5-safe process management coverage from CPython test_os."""

import os


pid = os.fork()
if pid == 0:
    os._exit(0)

child_pid, status = os.waitpid(pid, 0)
assert child_pid == pid
assert status == 0

print("test_process: fork/waitpid PASS")
