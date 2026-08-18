"""PS5 adaptations of CPython test_tracemalloc and multiprocessing imports."""

import tracemalloc
import ctypes
import mmap
import os
import signal
import sys


tracemalloc.start()
values = [bytearray(128) for _ in range(8)]
current, peak = tracemalloc.get_traced_memory()
assert current > 0
assert peak >= current
tracemalloc.stop()
assert not tracemalloc.is_tracing()
del values
assert ctypes.sizeof(ctypes.c_int) >= 2
assert ctypes.c_int(42).value == 42
assert ctypes.Structure is not None
assert ctypes.POINTER(ctypes.c_int) is not None

test_base = "/data/python" if sys.platform.startswith("freebsd") else os.getcwd()
path = os.path.join(test_base, ".mmap_test_{0}".format(os.getpid()))
fd = os.open(path, os.O_CREAT | os.O_RDWR, 0o600)
try:
    os.write(fd, b"mapped")
    try:
        mapped = mmap.mmap(fd, 6)
    except OSError as error:
        assert error.errno == 45
        print("test_diagnostics: mmap unavailable (ENOTSUP)")
    else:
        try:
            assert mapped[:6] == b"mapped"
            mapped[0:6] = b"update"
        finally:
            mapped.close()
finally:
    os.close(fd)
    os.remove(path)

called = []
previous = signal.getsignal(signal.SIGINT)


def handler(signum, frame):
    called.append(signum)


signal.signal(signal.SIGINT, handler)
assert signal.getsignal(signal.SIGINT) is handler
signal.signal(signal.SIGINT, previous)
assert signal.getsignal(signal.SIGINT) is previous

try:
    import _multiprocessing
    multiprocessing_available = True
except ImportError:
    multiprocessing_available = False

try:
    import _posixshmem
    posixshmem_available = True
except ImportError:
    posixshmem_available = False

try:
    import subprocess
    subprocess_available = True
except ImportError:
    subprocess_available = False

print("test_diagnostics: tracemalloc PASS")
print("test_diagnostics: _multiprocessing", multiprocessing_available)
print("test_diagnostics: _posixshmem", posixshmem_available)
print("test_diagnostics: subprocess", subprocess_available)
