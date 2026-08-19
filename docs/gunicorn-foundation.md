# Gunicorn foundation audit

This audit applies to the pinned CPython **3.14.7** runtime in
`CPYTHON_VERSION.txt` and the synchronous pre-fork worker model used by
Gunicorn.

## What is now validated

`tests/stdlib/test_gunicorn_foundation.py` is a bounded, import-light smoke
test based on the pinned CPython `Lib/test/test_os.py`, `test_signal.py`, and
`test_socket.py` behavior checks. On PS5 it verifies:

- an arbiter-created IPv4 listening socket can be inherited by forked workers;
- a worker can wrap and own its inherited descriptor with
  `socket.socket(fileno=...)` and serve a request. PS5 currently has no
  working `dup()`/`dup2()`, so the child must close its inherited copy only
  after the worker wrapper is done;
- two pre-fork workers can independently accept and complete requests;
- the arbiter can send `SIGTERM` and reap a blocked worker with `waitpid()`;
- `SIGCHLD` handlers can be installed and restored around worker reaping.

The test uses only loopback IPv4, one request per worker, and bounded blocking
operations. It is skipped by the desktop host suite because Windows does not
provide the POSIX `fork()` boundary being measured.

## Remaining Gunicorn foundation

The standard-library boundary is sufficient for a small synchronous
pre-fork prototype, but it is not yet a claim that the third-party Gunicorn
package is bundled. The remaining work is:

- bundle and audit Gunicorn's official release dependencies (`gunicorn` itself
  is not part of CPython);
- implement an arbiter loop with worker-count, restart, and graceful-shutdown
  policy;
- validate inherited listener behavior across repeated worker generations;
- add timeout, abnormal-exit, and signal-escalation handling;
- complete descriptor lifecycle checks for worker stdout/stderr and access
  logging;
- keep `spawn`, `forkserver`, `ProcessPoolExecutor`, queues, and shared-memory
  paths out of scope until their missing process-launch primitives are ready.

The current PS5 `subprocess` limitation remains important: Gunicorn's normal
CLI entry point and configuration discovery may invoke external processes or
shell helpers. The first useful target is therefore an embedded, synchronous
WSGI launcher that receives an application callable and an already-created
listener, followed by the package's ordinary arbiter once its imports and
worker lifecycle are validated.

## Reference basis

The process model follows Gunicorn's documented arbiter/worker design, while
the portable OS assertions are kept tied to the pinned CPython tests rather
than copied from third-party test internals. See `docs/wsgi-server-audit.md`
for the preceding single-process and threaded `wsgiref` validation.
