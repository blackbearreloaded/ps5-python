# Third-party web stack status

This document covers vendored third-party packages. It is separate from the
CPython **3.14.7** standard-library inventory; Gunicorn, Flask, and Werkzeug are
not standard-library modules.

## Gunicorn 23.0.0

The official pure-Python Gunicorn package is vendored under
`third_party/gunicorn` and bundled for the supported synchronous path. Its
arbiter can bind an IPv4 TCP listener, fork sync workers, inherit a listener
through `fd://`, handle `SIGTERM`/`SIGCHLD`, and reap workers with `waitpid()`.
Loopback WSGI requests pass in `tests/integration/test_gunicorn.py`,
`tests/integration/test_gunicorn_server.py`, and
`tests/integration/test_gunicorn_foundation.py`.

Daemon/re-exec mode, Unix-domain sockets, distribution-metadata plugin entry
points, and gevent/eventlet workers remain outside the PS5 contract. The
remaining lifecycle boundary is documented in
[`gunicorn-foundation.md`](gunicorn-foundation.md).

## Flask 3.1.3 and Werkzeug 3.1.8

The pure-Python Flask closure is vendored separately from the standard library:

| Package | Version | Supported PS5 surface |
| --- | --- | --- |
| Flask | 3.1.3 | Routing, test client, JSON responses, Jinja rendering, signed sessions, and Gunicorn sync serving |
| Werkzeug | 3.1.8 | WSGI request/response objects and WSGI test client |
| Jinja2 / MarkupSafe | 3.1.6 / 3.0.3 | Template rendering and escaping; native MarkupSafe speedups omitted |
| ItsDangerous / Click / Blinker | 2.2.0 / 8.2.1 / 1.9.0 | Flask dependency imports and signed-session support |

`tests/integration/test_flask.py` exercises the Flask and Werkzeug clients,
JSON handling, template escaping, signed cookies, and a real Flask route served
by Gunicorn. The development reloader/debugger, dotenv discovery,
multi-process development server, and optional async workers are not enabled.

## Test basis

The integration adapters cite the pinned CPython 3.14.7 WSGI, HTTP, cookie,
and cookie-jar tests in [`tests/UPSTREAM_TESTS.md`](../tests/UPSTREAM_TESTS.md).
They validate third-party packages at the standard-library WSGI boundary; they
do not reclassify those packages as CPython modules.
