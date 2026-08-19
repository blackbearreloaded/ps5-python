# WSGI server feasibility on PS5

This audit applies to the pinned CPython **3.14.7** runtime in
`CPYTHON_VERSION.txt`.

## Result

The smallest reference web-server target is the official CPython
`wsgiref.simple_server` package. It is a pure-Python WSGI reference server and
does not require Flask, Werkzeug, Waitress, Gunicorn, or a package installer.
The existing PS5 runtime already provides its important lower-level pieces:

- `http.server` and `socketserver` for HTTP parsing and the listening socket;
- `socket`, `select`, and `threading` for TCP and request handling;
- `urllib.parse`, `io`, `platform`, `os`, and `time` for the WSGI adapters;
- `email`/HTTP header support and the standard typing/collections helpers.

The initial target was a single-process WSGI server, followed by an optional
threaded variant. Both are now validated. A threaded WSGI server can be expressed using the
same standard-library mix-in used by `http.server`:

```python
from socketserver import ThreadingMixIn
from wsgiref.simple_server import WSGIServer, WSGIRequestHandler, make_server

class ThreadedWSGIServer(ThreadingMixIn, WSGIServer):
    daemon_threads = True

httpd = make_server(
    "0.0.0.0", 8000, application,
    server_class=ThreadedWSGIServer,
    handler_class=WSGIRequestHandler,
)
httpd.serve_forever()
```

The focused `tests/stdlib/test_wsgiref.py` smoke test uses loopback, a small WSGI
application, and a controlling thread to verify the status line, headers,
body, WSGI environment, and request shutdown. Its threaded companion and the
Flask/Gunicorn integration test now provide the next framework boundary.

## Limits and sequencing

`wsgiref` is a reference/debug server, not a hardened production server. The
current PS5 runtime also has these known constraints:

- IPv6 is disabled; bind and test with IPv4/loopback addresses.
- `subprocess` child execution is still unavailable for arbitrary external
  commands, but Gunicorn's own in-process `fork()` master/worker lifecycle is
  validated for the sync TCP worker. `ProcessPoolExecutor` and spawn-based
  workers remain out of scope.
- The socket and HTTP layers have import and smoke-test coverage, but not the
  complete upstream HTTP server, keep-alive, malformed-request, timeout, and
  load coverage.
- TLS is available through the existing OpenSSL build, but HTTPS WSGI should
  be tested separately with explicit certificate paths.
- `wsgiref` should be copied from the pinned upstream `Lib/wsgiref` tree as a
  complete package: `handlers.py`, `headers.py`, `simple_server.py`, `types.py`,
  `util.py`, and `validate.py`.

Gunicorn 23.0.0 is now vendored for the supported sync pre-fork path; see
`docs/gunicorn-foundation.md` for its lifecycle contract and limitations.
Flask 3.1.3 and its pure-Python dependency closure are now bundled and tested
through Gunicorn's sync WSGI worker; the framework-specific coverage is listed
in `docs/web-stack-status.md`. Flask's development server is not the production
target here: its reloader/debugger and multi-process paths remain disabled.
Waitress remains a possible later candidate for a separate threaded server.

## Upstream test basis

The focused tests are adapted from the pinned CPython tests
`Lib/test/test_wsgiref.py`, `Lib/test/test_httpservers.py`, and the WSGI server
helpers used by `Lib/test/test_asyncio/utils.py`. Preserve the protocol and
lifecycle assertions while omitting subprocess, IPv6, TLS-certificate-store,
and full stress portions that are outside the current PS5 runtime.
