"""PS5 adaptation of CPython Lib/test/test_io.py."""

import io


binary = io.BytesIO()
assert binary.write(b"hello") == 5
assert binary.getvalue() == b"hello"
assert binary.tell() == 5
binary.seek(0)
assert binary.read() == b"hello"
binary.seek(0)
assert binary.read(2) == b"he"
assert binary.read() == b"llo"

text = io.StringIO()
assert text.write("hello") == 5
assert text.getvalue() == "hello"
assert text.tell() == 5
text.seek(0)
assert text.read() == "hello"
text.seek(0)
assert text.readline() == "hello"

print("test_io: PASS")
