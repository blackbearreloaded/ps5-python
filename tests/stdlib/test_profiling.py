"""PS5 adaptations of CPython timeit, dis, tracemalloc, and struct tests."""

import dis
import struct
import timeit
import tracemalloc


assert timeit.timeit("x = 1 + 1", number=10) >= 0
instructions = list(dis.get_instructions(lambda value: value + 1))
assert instructions
assert any(instruction.opname in ("RETURN_VALUE", "RETURN_CONST") for instruction in instructions)
assert struct.unpack("!I", struct.pack("!I", 42))[0] == 42
tracemalloc.start()
assert tracemalloc.is_tracing()
tracemalloc.stop()

print("test_profiling: PASS")
