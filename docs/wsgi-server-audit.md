# WSGI server feasibility on PS5

This audit applies to the pinned CPython **3.14.7** runtime in
`CPYTHON_VERSION.txt`.

## Result

The smallest useful web-server target is the official CPython
`wsgiref.simple_server` package. It is a pure-Python WSGI reference server and
does not require Flask, Werkzeug, Waitress, Gunicorn, or a package installer.
The existing PS5 runtime already provides its important lower-level pieces:

- `http.server` and `socketserver` for HTTP parsing and the listening socket;
- `socket`, `select`, and `threading` for TCP and request handling;
- `urllib.parse`, `io`, `platform`, `os`, and `time` for the WSGI adapters;
- `email`/HTTP header support and the standard typing/collections helpers.

The initial target should be a single-process WSGI server, followed by an
optional threaded variant. A threaded WSGI server can be expressed using the
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

The first PS5 smoke test should use loopback, a small WSGI application, and a
second client request while the first handler is blocked. It should verify the
status line, headers, body, WSGI environment, request shutdown, and
`server.shutdown()`/`server_close()` from a controlling thread. This is the
minimum evidence needed before trying a real framework.

## Limits and sequencing

`wsgiref` is a reference/debug server, not a hardened production server. The
current PS5 runtime also has these known constraints:

- IPv6 is disabled; bind and test with IPv4/loopback addresses.
- `subprocess` child execution is unavailable, so Gunicorn's normal master and
  worker lifecycle cannot run yet. `ProcessPoolExecutor` and pre-fork workers
  are out of scope for this stage.
- The socket and HTTP layers have import and smoke-test coverage, but not the
  complete upstream HTTP server, keep-alive, malformed-request, timeout, and
  load coverage.
- TLS is available through the existing OpenSSL build, but HTTPS WSGI should
  be tested separately with explicit certificate paths.
- `wsgiref` should be copied from the pinned upstream `Lib/wsgiref` tree as a
  complete package: `handlers.py`, `headers.py`, `simple_server.py`, `types.py`,
  `util.py`, and `validate.py`.

Waitress is a reasonable later candidate for a more robust threaded server, but
it is third-party code and is not present in this source tree. Flask's
development server is also not an independent target: Flask brings Werkzeug,
Jinja, Click, and additional packaging/runtime dependencies. Neither should
be added until the standard-library WSGI smoke test passes on PS5.

## Upstream test basis

The focused test should be adapted from the pinned CPython tests
`Lib/test/test_wsgiref.py`, `Lib/test/test_httpservers.py`, and the WSGI server
helpers used by `Lib/test/test_asyncio/utils.py`. Preserve the protocol and
lifecycle assertions while omitting subprocess, IPv6, TLS-certificate-store,
and full stress portions that are outside the current PS5 runtime.
