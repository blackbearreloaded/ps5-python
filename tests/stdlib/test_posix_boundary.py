"""PS5 adaptation of safe process-boundary checks from CPython test_os."""

import os
import signal


assert os.getpid() > 0
variable = "CPYTHONPS5_POSIX_TEST"
os.putenv(variable, "present")
os.environ[variable] = "present"
try:
    assert os.getenv(variable) == "present"
finally:
    del os.environ[variable]
    os.unsetenv(variable)

read_fd, write_fd = os.pipe()
try:
    assert os.write(write_fd, b"pipe") == 4
    assert os.read(read_fd, 4) == b"pipe"
finally:
    os.close(write_fd)
    os.close(read_fd)

assert signal.SIGINT > 0
assert signal.getsignal(signal.SIGINT) is not None

print("test_posix_boundary: PASS")
