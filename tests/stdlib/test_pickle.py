"""CPython 3.14.7 Tier 4 data-structure and archive smoke tests.

Adapted from Lib/test/test_pickle.py, test_struct.py, test_bisect.py,
test_heapq.py, test_array.py, test_fractions.py, test_base64.py,
test_gzip.py, test_zipfile, test_tarfile.py, test_xml_etree.py,
test_xml_dom_minidom.py, test_sax.py, test_glob.py, test_fnmatch.py, and
test_sqlite3.
"""

import array
import base64
import bisect
import decimal
import fnmatch
import fractions
import glob
import gzip
import heapq
import io
import operator
import os
import pickle
import struct
import sqlite3
import tarfile
import tempfile
import zipfile
import zlib
from xml.dom import minidom
from xml.sax import parseString
from xml.sax.handler import ContentHandler


value = {"name": "PS5", "values": [1, 2, 3]}
assert pickle.loads(pickle.dumps(value)) == value
assert struct.unpack("!I", struct.pack("!I", 0x12345678))[0] == 0x12345678

ordered = [1, 3, 5]
bisect.insort(ordered, 4)
assert ordered == [1, 3, 4, 5]

heap = []
for item in (3, 1, 2):
    heapq.heappush(heap, item)
assert [heapq.heappop(heap) for _ in range(3)] == [1, 2, 3]

values = array.array("i", [1, 2, 3])
values.append(4)
assert values.tolist() == [1, 2, 3, 4]
assert operator.add(2, 3) == 5
assert decimal.Decimal("1.20") + decimal.Decimal("0.30") == decimal.Decimal("1.50")
assert fractions.Fraction(1, 3) + fractions.Fraction(1, 6) == fractions.Fraction(1, 2)

encoded = base64.b64encode(b"PS5 data")
assert base64.b64decode(encoded) == b"PS5 data"

payload = b"zlib stream payload\n"
compressed_payload = zlib.compress(payload)
assert zlib.decompress(compressed_payload) == payload
assert zlib.crc32(payload) == 0x300A8D65

compressed = io.BytesIO()
with gzip.open(compressed, "wb") as handle:
    handle.write(b"archive data\n")
compressed.seek(0)
with gzip.open(compressed, "rb") as handle:
    assert handle.read() == b"archive data\n"

archive = io.BytesIO()
with zipfile.ZipFile(archive, "w") as handle:
    handle.writestr("source.txt", b"archive data\n")
archive.seek(0)
with zipfile.ZipFile(archive) as handle:
    assert handle.read("source.txt") == b"archive data\n"

tar_buffer = io.BytesIO()
tar_info = tarfile.TarInfo("source.txt")
tar_info.size = len(b"archive data\n")
with tarfile.open(fileobj=tar_buffer, mode="w") as handle:
    handle.addfile(tar_info, io.BytesIO(b"archive data\n"))
tar_buffer.seek(0)
with tarfile.open(fileobj=tar_buffer) as handle:
    assert handle.extractfile("source.txt").read() == b"archive data\n"

if os.name != "nt":
    with tempfile.TemporaryDirectory(prefix="cpython-ps5-tier4-") as directory:
        source = os.path.join(directory, "source.txt")
        with open(source, "wb") as handle:
            handle.write(b"archive data\n")
        assert glob.glob(os.path.join(directory, "*.txt")) == [source]
else:
        print("test_pickle: filesystem glob checks skipped on host")
assert fnmatch.fnmatch("source.txt", "*.txt")

connection = sqlite3.connect(":memory:")
connection.execute("create table records (name text, value integer)")
connection.executemany("insert into records values (?, ?)", [("one", 1), ("two", 2)])
assert connection.execute(
    "select name, value from records order by value"
).fetchall() == [("one", 1), ("two", 2)]
connection.close()


document = minidom.parseString("<root><item>value</item></root>")
assert document.documentElement.tagName == "root"


class _ContentHandler(ContentHandler):
    def __init__(self):
        self.characters_seen = []

    def characters(self, content):
        self.characters_seen.append(content)


handler = _ContentHandler()
parseString("<root>value</root>", handler)
assert "value" in handler.characters_seen

print("test_pickle: PASS")
