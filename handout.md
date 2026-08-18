# CPythonPS5 continuation handout

## Mission

CPythonPS5 is an experimental port of CPython to the jailbroken PS5 payload
environment. The long-term goal is a usable Python runtime for PS5 homebrew:
standalone scripts first, packaged Python applications next, and eventually
practical web servers and native launchers.

The current design is deliberately simple: one PS5 ELF, one external Python
script, and a small runtime bundle.

Project root:

`C:\Users\denis\Documents\PS5\workspace\CPythonPS5`

The pinned CPython checkout is outside Git at `upstream/cpython`. Recreate it
with `make source-fetch`. Builds use WSL and the installed PS5 payload SDK.
The normal test console is PS5 `192.168.4.30`, configurable with `PS5_HOST`.

## Completed work

### Core interpreter

The standalone artifact is `build/ps5/python.elf`; the static library is
`build/ps5/libpython3.14.a`. Core validation covers expressions, numbers,
strings, Unicode basics, lists, tuples, dictionaries, sets, functions,
closures, recursion, classes, inheritance, attributes, comprehensions,
generators, iterators, exceptions, try/finally, allocation churn, and GC
stress.

### Standard-library slice

The runtime bundle currently includes Python-level `os`, `stat`,
`genericpath`, `posixpath`, `abc`, `_collections_abc`, `io`, `socket`,
`enum`, `types`, `codecs`, minimal encodings, and `selectors`.
Native static `_socket` and `select` are enabled in
`tools/ps5-setup.local`.

Implemented filesystem APIs include listdir, mkdir, rmdir, open, read, write,
close, stat, fstat, lseek, rename, remove, path predicates, and os.urandom
through the available entropy fallback.

The working socket subset includes IPv4 TCP bind/listen/accept/connect,
send/recv/sendall, shutdown, makefile, timeouts, blocking mode, getsockname,
getpeername, SO_REUSEADDR, TCP_NODELAY, numeric/ASCII getaddrinfo, UDP
sendto/recvfrom, select.select, and a small selector implementation.

The web launcher uses libmicrohttpd, serves port 8090, lists packaged apps,
launches them, captures stdout/stderr, and streams logs over WebSockets.
`apps/socket_server` is the verified demo. It appears as **Socket Server
9091**, listens on 9091, prints received data, and replies `OK`:

```sh
printf 'hello from netcat\n' | nc 192.168.4.30 9091
```

### Current verification

The complete PS5 aggregate suite currently passes:

```
CPYTHON_CORE_SUITE: PASS (41 scripts)
```

It includes the core tests plus:

- `tests/stdlib/test_os.py`
- `tests/stdlib/test_time.py`
- `tests/stdlib/test_io.py`
- `tests/stdlib/test_socket.py`
- `tests/stdlib/test_select.py`
- `tests/stdlib/test_selectors.py`
- `tests/stdlib/test_process.py`
- `tests/stdlib/test_network.py`
- `tests/stdlib/test_posix_boundary.py`
- `tests/stdlib/test_ssl_hashlib.py`
- `tests/stdlib/test_thread_context.py`
- `tests/stdlib/test_data_formats.py`
- `tests/stdlib/test_import_runtime.py`
- `tests/stdlib/test_diagnostics.py`
- `tests/stdlib/test_data_structures.py`
- `tests/stdlib/test_profiling.py`

Tests are adapted from official CPython names and concepts; provenance is in
`tests/UPSTREAM_TESTS.md`. Standard-library status is tracked in
`docs/stdlib-status.md`.

## Build and deployment

From WSL at the project root:

```sh
make source-fetch
make host-build
make ps5-core
PS5_HOST=192.168.4.30 make ps5-test
PS5_HOST=192.168.4.30 make ps5-suite
```

Run a standalone script:

```sh
PS5_HOST=192.168.4.30 make ps5-run SCRIPT=examples/main.py
PS5_HOST=192.168.4.30 make ps5-run SCRIPT=tests/core_suite.py
```

Build/deploy the web launcher:

```sh
PS5_HOST=192.168.4.30 PS5_WEB_CHECK=0 make ps5-web
```

It serves:

```
http://192.168.4.30:8090/
```

Before redeploying, stop the old instance because replacing the ELF does not
stop an already-running process:

```sh
curl http://192.168.4.30:8090/api/shutdown
```

`ps5-core` and `ps5-web` build only. `ps5-test` uploads and runs the
aggregate suite. `ps5-suite` adds lifetime checks. The deployment root
defaults to `/data/python`; override it with
`PS5_RUNTIME_ROOT=/some/absolute/path`.

When adding a runtime module, update all three paths:
`tools/build_ps5.sh`, `tools/run_ps5.sh`, and `tools/run_ps5_web.sh`.
A module can exist in the local build and still be absent on the PS5 if its
upload step is missing.

## Known limitations

### Time

`time.sleep()` is exposed but reaches PS5 libc behavior returning
`ENOSYS`. Existing demos use a monotonic busy-wait workaround. A reliable
native PS5 sleep hook is still needed.

### OS and IO

`fork()` and `waitpid()` are verified for an immediate-exit child. `exec`,
spawn, system, and subprocess remain unavailable because ordinary libc ELF
launching and descriptor duplication are not supported. Complete upstream os
coverage is pending. IO currently
focuses on BytesIO and StringIO; file-backed buffered streams, full incremental
codec layers, and complete upstream IO coverage remain incomplete.

### Socket and DNS

The minimal IDNA codec in `tools/minimal_idna.py` supports ordinary ASCII
hostnames such as `google.com`. Unicode internationalized domain names are
unsupported because full IDNA/Punycode and unicodedata are not bundled.
IPv6, advanced UDP behavior, full fcntl/nonblocking semantics, advanced socket
options, complete upstream socket regression coverage, and live TLS remain
incomplete or unverified. OpenSSL and the Python `ssl` APIs are already linked
and import-tested.

## Recommended next steps

1. Add a selected CA bundle and certificate verification to the passing
   `tests/stdlib/test_tls_handshake.py` smoke test.
2. Build and integrate static zlib, then add gzip/content-encoding tests.
3. Bundle and test the higher-level `threading` wrapper.
4. Build a minimal HTTP server on the verified IPv4 selector layer.
5. Validate ctypes against a safe PS5-native test library or broker API.
6. Investigate the kernel-assisted ELF broker for subprocess-compatible
   workers.
7. Attempt a minimal Flask/Werkzeug/Jinja2/MarkupSafe application bundle.

## Development rules

- Use official CPython source as the compatibility reference.
- Preserve official test names and keep modules in separate tests.
- Prefer the smallest PS5-compatible implementation.
- Reuse CPython Python-level wrappers where possible.
- Use WSL for compilation and deployment.
- Run `make ps5-test` after interpreter or bundle changes.
- Update `docs/stdlib-status.md` for every standard-library change.
- Record unsupported behavior explicitly.
- Shut down the old web launcher before validating a redeploy.
- Use the web HTTP APIs to inspect status and captured logs.
- Do not clean up deployed test apps unless explicitly requested.

## Files to read first

```
README.md
PLAN.md
docs/stdlib-status.md
docs/testing.md
docs/web-launcher.md
docs/app-bundles.md
tools/build_ps5.sh
tools/run_ps5.sh
tools/run_ps5_web.sh
tools/ps5-setup.local
tools/minimal_idna.py
src/cpython_web_launcher.c
tests/core_suite.py
tests/UPSTREAM_TESTS.md
apps/socket_server/main.py
```

The repository has Git checkpoints through the current runtime and standard-
library expansion. Review generated files before committing follow-up changes.
