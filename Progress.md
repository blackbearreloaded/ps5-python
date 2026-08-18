# CPythonPS5 Progress

The runtime target is CPython **3.14.7**, pinned by `CPYTHON_VERSION.txt` to
upstream source commit `823f0323ee6ec1402088b73bce1a38473cac36dc`.

## Date

2026-08-18

## Follow-up

- Host validation now passes all 44 discovered scripts. Tests that require PS5
  capabilities (fork, `select.poll`, external DNS, and live TLS) skip cleanly
  on desktop Python instead of making the host baseline nondeterministic.
- Added `tests/stdlib/test_tls_handshake.py` as a separate live PS5 smoke test;
  the PS5 handshake now passes with certificate checking disabled. A selected
  PS5 CA bundle is the next TLS increment.
- PS5 aggregate suite passes with the profiling and concurrency wrappers:
  `CPYTHON_CORE_SUITE: PASS (43 scripts)`.

## Completed Today

### Build performance

- Default PS5 parallelism now uses WSL `nproc`; the current machine builds with
  `PS5_JOBS=24`.
- Added automatic compiler-cache selection with `ccache` and `sccache`.
- Added `PS5_CACHE=ccache|sccache|none` and retained `PS5_CCACHE=0`.
- Added opt-in `PS5_LINKER=mold`; LLVM `lld` remains the default.
- Launcher linking is skipped when none of its inputs changed.
- README documents the build acceleration options.

### Networking and DNS

- Added the packaged `apps/dns_demo` application.
- Verified `localhost` and external `google.com` DNS resolution on PS5.
- Existing IPv4 TCP and UDP support remains verified.
- Added non-blocking socket tests for `accept()` and `recv()`.
- Added `select.poll()` readiness tests.
- Existing `select.select()` and selector tests continue to pass.

### Core standard library

Static or bundled support now includes:

- `_sre` and `re`
- `_json` and `json`
- `_struct` and `struct`
- `math`
- `_codecs` and minimal encodings
- `unicodedata`
- `binascii`
- `csv` and `_csv`
- `decimal` and `_decimal`
- `pyexpat`, `_elementtree`, and `xml.etree.ElementTree`
- `base64`
- `warnings` and `_py_warnings`
- `collections`, `_collections`, and `deque`, `defaultdict`, `Counter`, and
  `namedtuple`
- `itertools`
- `heapq` and `_heapq`
- A PS5-compatible `dataclasses` subset
- `pathlib`, `stat`, `_stat`, `posixpath`, and `zipimport`
- `tempfile.TemporaryDirectory`, `NamedTemporaryFile`, and `mkstemp` using an
  explicit writable PS5 directory
- `timeit` and `dis`

### Security and hashing

- Built OpenSSL 3.5.2 statically for PS5 from a reproducible build script.
- Linked CPython `_ssl` and `_hashlib` into the executable.
- Verified OpenSSL version detection and TLS context creation.
- Verified MD5, SHA-256, SHA-3, and OpenSSL-backed BLAKE2 construction.
- Bundled and tested Python-level `hmac` using `_hashlib`.
- Built and tested `_random` and Python-level `random`.

### Concurrency and diagnostics

- `_thread` and `contextvars` are available and tested.
- Verified joinable thread creation, locks, thread identity, `ContextVar`,
  tokens, and context copying.
- `_tracemalloc` is statically linked with the official CPython
  `tracemalloc.py` snapshot/statistics wrapper and its bundled dependency
  closure.
- Verified tracing start/stop and traced-memory counters.
- `_multiprocessing` and `_posixshmem` are statically linked and import-tested.
- Bundled official `threading.py`, `concurrent.futures.ThreadPoolExecutor`,
  and the supported `multiprocessing` package surface. PS5 verifies thread
  pools, `multiprocessing.Pipe`, and `Process` with an explicit `fork`
  context; Queue/Semaphore/SharedMemory and ProcessPoolExecutor are recorded
  as platform limitations.
- Statically linked native `array` for multiprocessing reduction support.
- Verified concrete `pathlib.Path` operations and tempfile context cleanup;
  patched `shutil.rmtree` to use its PS5-compatible path-based fallback.
- `_ctypes` is statically linked against a PS5-built libffi 3.8.0 Unix SysV
  backend.
- Verified basic `ctypes.c_int` construction and sizing.
- Verified signal handler installation and restoration.
- Added and tested native `mmap` compilation and import behavior.

### POSIX and processes

- Verified `os.getpid()`, environment mutation, pipes, and descriptor I/O.
- Verified `fork()`, `_exit()`, and `waitpid()` with an immediate-exit child.
- Added `signal` wrapper support and safe signal inspection.

## Verification

The final PS5 aggregate run completed with:

```text
CPYTHON_CORE_SUITE: PASS (43 scripts)
```

The suite includes adapted tests based on the pinned CPython `Lib/test` tree.
Mappings are recorded in `tests/UPSTREAM_TESTS.md`.

Repeated lifetime validation also passes:

```text
CPYTHON_PS5_LIFETIME: PASS (3 process runs)
```

## Known Limitations

### Compression

`zlib`, `_bz2`, and `_lzma` are not implemented. Their external PS5
dependencies still need static builds and integration.

### IPv6

IPv6 remains disabled. Enabling it causes CPython's configure probe to reject
the PS5 SDK `getaddrinfo()` behavior. Networking is currently IPv4-only.

### TLS

OpenSSL is linked and a live HTTPS handshake now passes on PS5. Certificate
verification and CA-store selection remain unimplemented.

### Hashing

The dedicated HACL `_sha3`, `_blake2`, and native `_hmac` modules are not linked.
Public SHA-3, BLAKE2, and HMAC functionality is provided through OpenSSL and
the Python-level wrappers.

### Dataclasses

The bundled dataclasses implementation covers core decoration, generated
initialization, representation, equality, defaults, and frozen instances. It
does not implement every CPython decorator option or reflection helper.

### Tracemalloc

The full Python snapshot/statistics layer is now bundled and passes the PS5
aggregate suite. Long-running tracing, snapshot stress, and cross-process
behavior remain unverified.

### Multiprocessing and process pools

The official Python wrappers are bundled. Thread pools, `Pipe`, and an
explicit-fork `Process` pass on PS5. Queue/Semaphore fail because named
semaphores are unavailable; SharedMemory cannot start its resource tracker and
file-backed `mmap` is `ENOTSUP`; ProcessPoolExecutor and default
forkserver/spawn launching remain unsupported without subprocess/ELF
integration.

### mmap

The native `mmap` module is compiled and importable, but `mmap.mmap()` returns
`ENOTSUP` in the current PS5 payload.

### Temporary files

`tempfile` works when callers provide the writable `/data/python` directory.
The payload has no usable default `/tmp`-style directory, so default temp-file
creation and `gettempdir()` fail. `TemporaryDirectory` cleanup works through
the path-based `shutil.rmtree` fallback because fd-based `os.scandir` is
`ENOTSUP`.

### ctypes

Basic ctypes data types work. Loading arbitrary PS5 `.sprx` or `.so` libraries,
callbacks, and calling native PS5 APIs remain unverified.

### Subprocess and executable launching

`subprocess` is unavailable. Ordinary libc `execve()` cannot execute PS5 ELFs,
standard executable paths are unavailable in the payload filesystem, and
`dup()`/`dup2()` return `ENOTSUP`. A kernel-assisted native ELF broker based on
the `shsrv` resource project is still required.

### File and terminal APIs

Core open/read/write/lseek/stat operations work. Descriptor duplication,
terminal integration, advanced fcntl behavior, and complete file-backed IO
coverage remain incomplete.

## What Is Missing

- Certificate-verified HTTPS with an explicit PS5 CA-store strategy.
- Static zlib, bzip2, and xz dependencies.
- Full `multiprocessing` and subprocess integration.
- Kernel-assisted process/ELF launching and cross-process descriptor transfer.
- A complete ctypes native-library loading test.
- Full pathlib, dataclasses, SSL, and compression upstream test coverage;
  long-running profiling stress remains unverified.
- Queue/semaphore support, POSIX shared memory, and process-pool launching once
  the PS5 payload gains named semaphores, file-backed `mmap`, and an ELF broker.
- A complete HTTP server foundation suitable for Flask or Werkzeug.
- Flask, Werkzeug, Jinja2, and MarkupSafe dependency validation.

## Next Steps

1. Build and integrate static zlib, then add gzip/content-encoding tests.
2. Complete HTTPS handshake and certificate-store strategy using OpenSSL.
3. Add a minimal HTTP server using the verified IPv4 poll/event-loop layer.
4. Validate `ctypes` against a safe PS5-native test library or broker API.
5. Investigate the kernel-assisted ELF broker for subprocess-compatible workers.
6. Expand upstream-derived concurrency tests as each PS5 primitive becomes
   available, without weakening the documented PS5 subset.
7. Attempt a minimal Flask/Werkzeug/Jinja2/MarkupSafe application bundle.

## Git Checkpoints

- `a079ed0` Add DNS and core stdlib modules
- `3a84174` Expand PS5 POSIX boundary coverage
- `2d202a8` Validate PS5 fork and waitpid support
- `3369a40` Expand PS5 network event loop coverage
- `e44beff` Add static OpenSSL support to PS5 runtime
- `703297f` Add PS5 thread and contextvars coverage
- `e4aa369` Add XML CSV and decimal support
- `f60e624` Add pathlib and import runtime support
- `4fe380c` Add diagnostics and multiprocessing primitives
- `105ea8e` Build libffi and enable ctypes
- `c3302fc` Add collections algorithms and dataclasses
- `e65ca71` Add profiling and diagnostics wrappers
- `77543b3` Support multiprocessing SelectSelector alias
- `cbab5df` Mark unsupported PS5 process IPC explicitly
- `0dc9393` Add concurrency and multiprocessing runtime support
- `67fbbd1` Document fork process concurrency coverage
