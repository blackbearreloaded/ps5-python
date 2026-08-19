"""CPython 3.14.7-derived compression and persistence smoke tests."""

import bz2
import lzma
import os
import shelve
import tempfile

from dbm import dumb


payload = b"PS5 Tier 8 compression payload\n" * 4
assert bz2.decompress(bz2.compress(payload)) == payload
assert lzma.decompress(lzma.compress(payload)) == payload


# Adapted from Lib/test/test_dbm_dumb.py and test_shelve.py.  The pure-Python
# dumb backend keeps this coverage available even when a platform NDBM/GDBM
# library is not present.
with tempfile.TemporaryDirectory(prefix="tier8-shelve-") as root:
    path = os.path.join(root, "records")
    backend = dumb.open(path, "c")
    backend[b"answer"] = b"42"
    assert backend[b"answer"] == b"42"
    backend.close()

    backend = dumb.open(path, "c")
    with shelve.Shelf(backend) as shelf:
        shelf["message"] = {"value": 42}
        assert shelf["message"] == {"value": 42}

print("test_tier8_compression: PASS")
