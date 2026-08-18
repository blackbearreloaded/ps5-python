"""PS5 adaptations of CPython test_tracemalloc and multiprocessing imports."""

import tracemalloc


tracemalloc.start()
values = [bytearray(128) for _ in range(8)]
current, peak = tracemalloc.get_traced_memory()
assert current > 0
assert peak >= current
tracemalloc.stop()
assert not tracemalloc.is_tracing()
del values

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

print("test_diagnostics: tracemalloc PASS")
print("test_diagnostics: _multiprocessing", multiprocessing_available)
print("test_diagnostics: _posixshmem", posixshmem_available)
