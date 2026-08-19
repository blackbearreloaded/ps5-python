# Gunicorn foundation audit

This audit applies to the pinned CPython **3.14.7** runtime in
`CPYTHON_VERSION.txt` and the synchronous pre-fork worker model used by
Gunicorn.

## What is now validated

Gunicorn 23.0.0 is vendored under `third_party/gunicorn` and copied into the
CPython **3.14.7** PS5 runtime bundle. The supported path is the official
sync worker with an IPv4 TCP listener. `tests/stdlib/test_gunicorn_server.py`
is adapted from the pinned CPython `Lib/test/test_wsgiref.py`,
`test_httpservers.py`, `test_os.py`, and `test_signal.py` behavior checks. On
PS5 it verifies:

- the Gunicorn arbiter creates a TCP listener and forks a sync worker;
- the worker parses and serves a real loopback WSGI request;
- the master receives `SIGTERM`, shuts down the worker, and exits cleanly;
- an already-open `fd://` listener is handed to the arbiter without
  `socket.fromfd()`; PS5 workers wrap inherited descriptors directly;
- the lower-level listener test also covers two independent pre-fork workers,
  `waitpid()` reaping, and `SIGCHLD` installation/restoration.

The test uses only loopback IPv4, one request per worker, and bounded blocking
operations. It is skipped by the desktop host suite because Windows does not
provide the POSIX `fork()` boundary being measured.

## Deliberate PS5 limits

The ordinary bind/listen, pre-fork, sync request, and TERM/CHLD supervision
path is now usable for an embedded or CLI-loaded WSGI application. The
following upstream features remain intentionally outside the PS5 contract:

- daemonization and USR2 re-exec, which require descriptor duplication and a
  second executable launch;
- Unix-domain sockets, because the PS5 socket build has no `AF_UNIX`;
- distribution metadata and entry-point plugin discovery (the runtime carries
  `importlib.metadata`, but the PS5 FTP filesystem does not deploy dotted
  wheel metadata directories);
- gevent, eventlet, and other optional asynchronous worker classes;
- process-pool, spawn, and forkserver integrations, which are separate from
  Gunicorn's forked sync workers.

Gunicorn's timeout, worker replacement, HUP reload, and signal escalation code
is retained from the official release; expand the bounded PS5 tests before
advertising those less common control paths for production workloads.

## Reference basis

The process model follows Gunicorn's documented arbiter/worker design, while
the portable OS assertions are kept tied to the pinned CPython tests rather
than copied from third-party test internals. See `docs/wsgi-server-audit.md`
for the preceding single-process and threaded `wsgiref` validation.
