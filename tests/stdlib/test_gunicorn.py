"""Gunicorn 23.0.0 import and sync HTTP parser smoke checks.

The parser assertions follow the request/WSGI boundary cases in the pinned
CPython 3.14.7 ``Lib/test/test_wsgiref.py`` and ``test_httpservers.py``.  A
full pre-fork lifecycle is intentionally covered by the PS5 process tests,
not duplicated here.
"""

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "third_party"))

import gunicorn
from gunicorn.config import Config
from gunicorn.http.parser import RequestParser
from gunicorn.workers.sync import SyncWorker
from gunicorn import util


assert gunicorn.__version__ == "23.0.0"
assert util.load_class("sync") is SyncWorker

cfg = Config()
request = next(RequestParser(
    cfg,
    [
        b"GET /hello?name=ps5 HTTP/1.1\r\n"
        b"Host: example.test\r\n"
        b"Content-Length: 0\r\n"
        b"\r\n",
    ],
    ("127.0.0.1", 1234),
))

assert request.method == "GET"
assert request.uri == "/hello?name=ps5"
assert request.path == "/hello"
assert request.query == "name=ps5"
assert ("HOST", "example.test") in request.headers
assert request.body.read() == b""

print("test_gunicorn: PASS")
