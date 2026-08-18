"""PS5 adaptations of CPython Tier 2 utility-module tests."""

import argparse
import copy
import csv
import enum
import hashlib
import io
import logging
import logging.config
import logging.handlers
import pprint
import random
import shutil
import subprocess
import sys
import tempfile
import traceback
import unittest
from urllib.parse import parse_qs, urlencode, urlparse
from urllib.request import Request


# CPython Lib/test/test_argparse.py, test_logging.py, test_shutil.py,
# test_random.py, test_copy.py, test_enum.py, test_csv.py, test_subprocess.py,
# test_urllib.py, test_hashlib.py, test_io.py, test_traceback.py, test_pprint.py,
# and test_unittest.py.
parser = argparse.ArgumentParser(prog="tier2", add_help=False)
parser.add_argument("--count", type=int, default=1)
assert parser.parse_args(["--count", "3"]).count == 3

log_stream = io.StringIO()
logger = logging.getLogger("cpython-ps5-tier2")
logger.handlers.clear()
logger.setLevel(logging.INFO)
handler = logging.StreamHandler(log_stream)
logger.addHandler(handler)
logger.info("ready")
handler.flush()
assert "ready" in log_stream.getvalue()
logger.removeHandler(handler)
assert logging.handlers.MemoryHandler is not None
assert logging.config.dictConfig is not None

if sys.platform.startswith("win32"):
    print("test_tier2: tempfile/shutil file checks skipped on host")
else:
    with tempfile.TemporaryDirectory() as directory:
        source = directory + "/source.txt"
        target = directory + "/target.txt"
        with open(source, "w", encoding="utf-8") as handle:
            handle.write("copy")
        shutil.copyfile(source, target)
        with open(target, encoding="utf-8") as handle:
            assert handle.read() == "copy"

rng = random.Random(7)
assert rng.choice(["PS5"]) == "PS5"
assert copy.deepcopy({"items": [1, 2]}) == {"items": [1, 2]}


class Color(enum.Enum):
    RED = 1


assert Color.RED.value == 1
csv_buffer = io.StringIO()
csv.writer(csv_buffer).writerow(["name", "value"])
assert list(csv.reader(io.StringIO(csv_buffer.getvalue()))) == [["name", "value"]]
assert hashlib.sha256(b"PS5").hexdigest() == (
    "215a8ba2edc6187348afcd37a61f6992afd326f2f5dd5b8b579d4d88fca4f94e"
)
assert io.BytesIO(b"PS5").read() == b"PS5"
assert "ValueError" in "".join(traceback.format_exception_only(ValueError("bad")))
assert "{'name': 'PS5'}" in pprint.pformat({"name": "PS5"})

query = urlencode({"q": "PS5", "page": 1})
assert parse_qs(urlparse("https://example.com/search?" + query).query)["q"] == ["PS5"]
assert Request("https://example.com/").full_url == "https://example.com/"

case = unittest.TestCase()
case.assertEqual(2 + 2, 4)

if sys.platform.startswith("freebsd"):
    try:
        subprocess.run(["true"], check=True)
    except (NotImplementedError, OSError):
        print("test_tier2: subprocess execution unavailable")
    else:
        raise AssertionError("PS5 subprocess unexpectedly executed a child ELF")
else:
    completed = subprocess.run(
        [sys.executable, "-c", "print('subprocess')"],
        check=True,
        capture_output=True,
        text=True,
    )
    assert completed.stdout.strip() == "subprocess"

print("test_tier2: PASS")
