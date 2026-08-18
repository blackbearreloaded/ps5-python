"""PS5 adaptations of CPython tests for core stdlib extension modules."""

import json
import math
import re
import struct
import unicodedata


payload = {"name": "PS5", "values": [1, 2, 3]}
assert json.loads(json.dumps(payload)) == payload
assert re.search(r"^PS5-(\d+)$", "PS5-5").group(1) == "5"
assert struct.unpack("<I", struct.pack("<I", 0x12345678))[0] == 0x12345678
assert math.isclose(math.sqrt(2.0) ** 2, 2.0, rel_tol=1e-12)
assert unicodedata.category("A") == "Lu"
assert "_codecs" in __import__("sys").builtin_module_names
assert "_sre" in __import__("sys").builtin_module_names
assert "_json" in __import__("sys").builtin_module_names
assert "_struct" in __import__("sys").builtin_module_names
assert "math" in __import__("sys").builtin_module_names

print("test_core_modules: PASS")
