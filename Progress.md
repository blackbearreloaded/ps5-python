# CPythonPS5 Progress

The runtime target is CPython **3.14.7**, pinned by `CPYTHON_VERSION.txt` to
upstream source commit `823f0323ee6ec1402088b73bce1a38473cac36dc`.

## Date

2026-08-18

## Follow-up

- Host validation now passes all 59 discovered scripts. Tests that require PS5
  capabilities (fork, `select.poll`, external DNS, and live TLS) skip cleanly
  on desktop Python instead of making the host baseline nondeterministic.
- Added `tests/stdlib/test_tls_handshake.py` as a separate live PS5 smoke test;
  the PS5 handshake now passes with certificate checking disabled. A selected
  PS5 CA bundle is the next TLS increment.
- PS5 aggregate suite passes with the profiling, concurrency, Tier 3, Tier 4,
  Tier 5, Tier 6, Tier 7, and feasible Tier 8 wrappers:
  `CPYTHON_CORE_SUITE: PASS (58 scripts)`.

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
- `tempfile.TemporaryDirectory`, `NamedTemporaryFile`, and `mkstemp` using the
  PS5-managed `/user/temp` directory
- `sys`, `datetime`, and `typing` (TypeVar, Generic, Protocol, aliases, and
  cast, TypedDict, and basic annotation introspection) with native `_datetime`
  and `_typing` support
- `functools.lru_cache` and `functools.partial`
- `timeit` and `dis`
- Tier 2 utility wrappers: `argparse`, official `logging`, `shutil`, `random`,
  `copy`, `enum`, `csv`, `unittest`, patched-import `subprocess`, `urllib`,
  `hashlib`, `io`, `traceback`, and `pprint`
- Tier 3 concurrency/networking wrappers: official `asyncio`, `threading`,
  `multiprocessing`, `concurrent.futures`, `socket`, `ssl`, `http`, `queue`,
  `select`, and `signal`
- Tier 4 data/format wrappers: `sqlite3`, `pickle`, `struct`, `bisect`,
  `heapq`, `array`, `operator`, `decimal`, `fractions`, `zlib`, `gzip`,
  `zipfile`, `tarfile`, `base64`, `xml`, `tempfile`, `glob`, and `fnmatch`
- Tier 5 metaprogramming/inspection wrappers: `inspect`, `ast`, `dis`,
  `importlib`, `abc`, `contextlib`, `gc`, `site`, `sysconfig`, `weakref`,
  `codecs`, and `types`
- Tier 6 security/i18n/text/POSIX wrappers: `secrets`, `hmac`, `getpass`,
  `gettext`, `locale`, `unicodedata`, `string`, `textwrap`, `difflib`,
  `mimetypes`, `uuid`, `stat`, `filecmp`, `termios`, `tty`, `fcntl`, and
  `resource`
- Tier 7 developer-tool wrappers: `pdb`, `timeit`, `cProfile`, `profile`,
  `pstats`, `tracemalloc`, `doctest`, `py_compile`, `compileall`, `code`,
  `codeop`, `readline`, and `rlcompleter`
- Tier 8 feasible utility wrappers: `graphlib`, `statistics`, `cmath`,
  `ipaddress`, `colorsys`, `calendar`, `zoneinfo`, `wave`, `binascii`,
  `ftplib`, `poplib`, `imaplib`, `smtplib`, `mailbox`, `email`, `shelve`, and
  pure `dbm.dumb`; named timezone data remains a deployment responsibility.
- Tier 8 compression support: static bzip2 1.0.8 and xz/liblzma 5.6.3 with
  native `_bz2` and `_lzma`; round-trip tests pass on host and PS5.
- Tier 8 omissions: `tkinter`, `curses`, native dbm backends, and `smtpd` are
  unavailable or infeasible on the PS5 target.
- Tier 9 core modules: interpreter-provided `__main__`, `builtins`, and
  `marshal`, native `_thread`, and official `__future__.py`/`copyreg.py`
  wrappers are available. Future feature metadata, marshal round trips,
  copyreg registration, and low-level thread locks/startup are tested from
  CPython's official `Lib/test` sources.

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

### Tier 2 utilities

- Bundled the pinned CPython 3.14.7 implementations and recursive pure-Python
  dependencies for all requested Tier 2 modules.
- Expanded `logging` to the complete official package surface shipped by the
  pin (`logging`, `logging.handlers`, and `logging.config`) plus its
  `socketserver` dependency.
- Extended the PS5 dataclasses foundation with `MISSING`, `Field`, `field()`,
  keyword-only dataclasses, default factories, generated field metadata, and
  `__post_init__` so the official logging colorization dependency imports.
- Patched only the `_posixsubprocess` import boundary in official
  `subprocess.py`; the module imports and reports the platform execution limit
  instead of pretending child ELF execution works.
- Added `tests/stdlib/test_tier2.py`, adapted from the pinned CPython
  `Lib/test/test_*.py` files, and recorded every requested module's gaps in
  `docs/stdlib-status.md`.

### Tier 3 concurrency and networking

- Bundled the complete pinned CPython 3.14.7 `Lib/asyncio` package and its
  official `html` and `mimetypes` dependencies required by `http.server`.
- Verified an asyncio event loop, coroutine scheduling, `asyncio.Queue`,
  synchronized `queue.Queue`, and thread-pool execution on PS5.
- Added `tests/stdlib/test_tier3.py`, adapted from the pinned asyncio,
  threading, multiprocessing, futures, socket, SSL, HTTP, queue, select, and
  signal tests; documented all platform ceilings in `docs/stdlib-status.md`.

### Tier 4 data structures and formats

- Built and linked static zlib 1.3.1 and SQLite 3.46.1 dependencies for PS5.
- Linked native `zlib` and `_sqlite3`, bundled official SQLite/archive/XML
  wrappers, and closed the cp437 codec dependency required by ZIP metadata.
- Added `tests/stdlib/test_tier4_formats.py`, adapted from the pinned CPython
  pickle, struct, zlib, gzip, ZIP, TAR, SQLite, XML, glob, and fnmatch tests.

### Tier 5 metaprogramming and inspection

- Bundled the official `inspect`, `ast`, and `dis` implementations, including
  AST unparse support and bytecode metadata.
- Completed the importlib/ABC package closure and verified finder, loader,
  relative-import, abstract-method, and virtual-subclass behavior.
- Bundled `contextlib`, `site`, and `_sitebuiltins`; added the official
  `sysconfig` package with a static-build fallback for missing generated data.
- Added focused tests derived from the pinned CPython AST, disassembly,
  inspection, importlib, ABC, contextlib, GC, site, sysconfig, weakref, codecs,
  and types tests.

### Tier 6 security, text, and POSIX utilities

- Bundled official security/i18n wrappers and verified secrets, HMAC, getpass,
  gettext, locale, and Unicode database behavior.
- Added official string/text/diff/MIME/UUID/stat/file comparison wrappers,
  including the recursive `platform` dependency required by `uuid`.
- Statically linked native `fcntl`, `resource`, and `termios`, and bundled
  official `tty.py`.
- Added focused tests derived from the pinned CPython Tier 6 test modules;
  Windows-only modules are intentionally excluded from this PS5 target.

### Tier 7 developer tools and profiling

- Statically linked native `_lsprof` and bundled official `cProfile.py`,
  `profile.py`, and `pstats.py`; extended the profiling test with deterministic
  call collection and report checks.
- Bundled official `doctest.py`, `py_compile.py`, `compileall.py`, `code.py`,
  and `codeop.py` with recursive compiler/test dependencies.
- Bundled `pdb.py`, `bdb.py`, `cmd.py`, and `rlcompleter.py`; added a PS5-safe
  history/completion `readline` layer because no linkable GNU/editline backend
  is available in the SDK.
- Added focused tests derived from CPython’s doctest, compile, code, pdb,
  readline, and profiler test modules.

## Verification

The final PS5 aggregate run completed with:

```text
CPYTHON_CORE_SUITE: PASS (58 scripts)
```

The suite includes adapted tests based on the pinned CPython `Lib/test` tree.
Mappings are recorded in `tests/UPSTREAM_TESTS.md`.

Repeated lifetime validation also passes:

```text
CPYTHON_PS5_LIFETIME: PASS (3 process runs)
```

## Known Limitations

### Compression

zlib 1.3.1, bzip2 1.0.8, and xz/liblzma 5.6.3 are statically linked and
tested through the native `zlib`, `_bz2`, and `_lzma` modules plus gzip, ZIP,
and TAR wrappers. Full upstream compression stress coverage remains pending.

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

### Typing and datetime

The official `typing.py`, `annotationlib.py`, `ast.py`, `keyword.py`, and
`datetime.py` wrappers are bundled over native `_typing` and `_datetime`.
Type variables, generic classes, protocols, aliases, casts, TypedDict,
annotation introspection, dates, aware datetimes, timezones, and timedeltas
pass the PS5 Tier 1 test. The timezone database and `zoneinfo` are not
included.

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

The PS5 launcher sets `TMPDIR=/user/temp`, a writable directory cleaned on
restart. Default tempfile creation and `gettempdir()` now use that location.
`TemporaryDirectory` cleanup works through the path-based `shutil.rmtree`
fallback because fd-based `os.scandir` is `ENOTSUP`.

### ctypes

Basic ctypes data types work. Loading arbitrary PS5 `.sprx` or `.so` libraries,
callbacks, and calling native PS5 APIs remain unverified.

### Subprocess and executable launching

`subprocess` imports through the patched official wrapper but execution is
unavailable. Ordinary libc `execve()` cannot execute PS5 ELFs,
standard executable paths are unavailable in the payload filesystem, and
`dup()`/`dup2()` return `ENOTSUP`. A kernel-assisted native ELF broker based on
the `shsrv` resource project is still required.

### File and terminal APIs

Core open/read/write/lseek/stat operations work. Descriptor duplication,
terminal integration, advanced fcntl behavior, and complete file-backed IO
coverage remain incomplete.

## What Is Missing

- Certificate-verified HTTPS with an explicit PS5 CA-store strategy.
- Full `multiprocessing` and subprocess integration.
- Full upstream regression coverage for the Tier 2 utility modules; the
  supported subset and each known gap are listed in `docs/stdlib-status.md`.
- Kernel-assisted process/ELF launching and cross-process descriptor transfer.
- A complete ctypes native-library loading test.
- Full pathlib, dataclasses, SSL, and compression upstream test coverage;
  long-running profiling stress remains unverified.
- Queue/semaphore support, POSIX shared memory, and process-pool launching once
  the PS5 payload gains named semaphores, file-backed `mmap`, and an ELF broker.
- A complete HTTP server foundation suitable for Flask or Werkzeug.
- Flask, Werkzeug, Jinja2, and MarkupSafe dependency validation.

## Next Steps

1. Expand bzip2/xz streaming and corruption coverage from the upstream tests.
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
- `65ee705` Bundle datetime and typing Tier 1 wrappers
- `4e323e9` Bundle typing annotation dependencies
- `7be1041` Bundle Tier 2 utility modules
- `a92c914` Extend dataclasses shim for official logging
- `a146ad2` Bundle encoding aliases dependency
- `8d6be17` Bundle official logging submodules
- `380b8a4` Bundle socketserver logging dependency
- `0b0662c` Document complete logging package surface
- `a9bde66` Bundle asyncio and add Tier 3 coverage
- `1067017` Bundle html dependency for http server
- `57378dc` Bundle mimetypes dependency for http server
- `6fab23f` Document Tier 3 concurrency and networking
- `5d13249` Build SQLite statically for PS5
- `00b8687` Bundle Tier 4 format and archive modules
- `1696173` Build static zlib for PS5 compression
- `e350b39` Pin zlib source checksum
- `96b5c92` Add Tier 4 format test to aggregate suite
- `c4d94e5` Exercise SQLite DB-API in Tier 4 test
- `6e31f92` Avoid fragile configure for Tier 4 native modules
- `6f1928e` Disable unavailable curses probes in PS5 configure
- `5386717` Disable unavailable intl configure probe
- `348be38` Use PS5 cross preprocessor during configure
- `3cca2c1` Upload sqlite3 runtime package to PS5
- `891ce0f` Bundle cp437 codec for ZIP archives
- `403cc00` Enable cp437 in PS5 codec registry
- `00ae5aa` Bundle ast unparse dependency and add Tier 5 inspection tests
- `2fc8b31` Document PS5 inspect source recovery limitation
- `739b451` Adapt frame source test for PS5 launcher
- `b6e35d9` Add Tier 5 importlib and abc smoke test
- `d29ec2e` Bundle Tier 5 runtime utility modules
- `629bd85` Adapt inspect module test for PS5 launcher
- `b11d62b` Add importlib test to Tier 5 aggregate
- `edba95f` Correct Tier 5 host suite count
- `ee60f46` Bundle getpass and test Tier 6 security i18n
- `3e2a32c` Add Tier 6 text and file utility tests
- `eaf2e95` Add native POSIX Tier 6 modules
- `ba353e0` Map POSIX Tier 6 tests to CPython
- `6d4fccb` Restore Tier 6 runtime upload closure
- `fae618a` Bundle platform dependency for uuid
- `1ac663a` Bundle Tier 7 compilation and doctest modules
- `de79eb7` Test Tier 7 profiling wrappers
- `5f7d4f4` Make tracemalloc profiling test runner-safe
- `4404a69` Document Tier 7 profiling support
- `7a6809b` Add PS5 interactive developer tools
- `3b2ef93` Complete Tier 7 test and runtime closure
- `60f8187` Relax readline completer identity test
- `e4623cf` Use portable tracemalloc filter pattern
- `529df8e` Document Tier 7 developer tools
- `7c9b690` Add feasible Tier 8 utility modules
- `e7efc20` Record Tier 8 utility checkpoint
- `8339b88` Clarify zoneinfo test limitation
- `0aed2fe` Add Tier 8 protocol and mailbox smoke tests
- `9696ab3` Bundle Tier 8 protocol and mail dependencies
- `450f7c2` Document Tier 8 protocol and mail support
- `8b6e4fd` Add Tier 8 compression and persistence support
- `911de47` Skip Tier 8 persistence filesystem check on host
- `1253240` Bundle importlib resources for zoneinfo
- `9efedc4` Bundle latin-1 codec for dbm dumb
- `aa6324d` Register Latin-1 in PS5 codec bootstrap
- `dd8146d` Handle PS5 Maildir filesystems without hard links
- Tier 9 core/internal coverage is tracked in the Tier 9 checkpoint below.
