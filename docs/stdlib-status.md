# Standard-library implementation status

This status report applies to the pinned CPython **3.14.7** source listed in
`CPYTHON_VERSION.txt`, not to an unspecified or moving CPython release.

This file is the source of truth for the PS5 standard-library subset. Every
new module implementation must record:

- the upstream CPython source used;
- APIs included and tested;
- APIs omitted by build-time platform guards;
- APIs present but limited or broken on PS5;
- the corresponding PS5 test file.

The goal is explicit compatibility reporting. A module must not be described
as complete merely because it imports successfully.

## Tier 1 daily-driver status

The requested CPython 3.14.7 Tier 1 modules are covered by the focused PS5
tests below. “Subset” means the public foundation works, while the omitted
APIs are recorded rather than implied to be complete.

| Module | PS5 status | Coverage and remaining gap |
| --- | --- | --- |
| `sys` | Built in and available | Interpreter state, streams, version, and arguments in `test_tier1.py`; full startup/configuration coverage remains pending. |
| `os` | Bundled | POSIX filesystem, environment, pipes, and process checks pass; advanced descriptor/process APIs remain limited. |
| `pathlib` | Bundled | Concrete `Path` and `PurePosixPath` operations pass; full upstream pathlib coverage is pending. |
| `typing` | Official wrapper plus native `_typing` | `TypeVar`, `Generic`, `Protocol`, aliases, `cast`, `TypedDict`, and basic annotation introspection pass; the complete upstream typing suite is pending. |
| `collections` | Bundled with native `_collections` | `deque`, `defaultdict`, `Counter`, and `namedtuple` pass; full upstream coverage is pending. |
| `dataclasses` | PS5-compatible subset | Core generation, equality, defaults, and frozen instances pass; advanced decorator/reflection options are omitted. |
| `json` | Official wrappers plus native `_json` | Encoding/decoding pass; complete upstream regression coverage is pending. |
| `datetime` | Official wrapper plus native `_datetime` | Date, timezone, timedelta, and aware-datetime operations pass; timezone database/`zoneinfo` is not bundled. |
| `time` | Native built in | Wall, monotonic, performance clocks, nanosecond clocks, and sleep support are tested. |
| `math` | Native static module | Core floating-point functions pass; complete upstream math coverage is pending. |
| `re` | Official wrapper plus native `_sre` | Pattern search and groups pass; complete upstream regex coverage is pending. |
| `functools` | Official wrapper | `lru_cache` and `partial` pass; broader helper coverage is pending. |
| `itertools` | Native static module | `count`, `islice`, and `permutations` pass; broader iterator coverage is pending. |

The combined smoke test is `tests/stdlib/test_tier1.py`, adapted from
`test_sys.py`, `test_typing.py`, and `datetimetester.py`; existing focused
tests cover the other rows.

## Tier 2 utility status

These modules use the pinned CPython 3.14.7 `Lib/` sources and the focused
`tests/stdlib/test_tier2.py` smoke test. The table records the supported
surface and the remaining PS5-specific gap for every requested module.

| Module | PS5 status | Included and tested | Missing or limited |
| --- | --- | --- | --- |
| `argparse` | Bundled | Parser construction, positional/optional arguments, and conversion | Full formatter, subparser, file-completion, and upstream regression coverage pending |
| `logging` | Official package bundled | Logger, level filtering, `StreamHandler`, and formatter output | File handlers, multiprocessing handlers, and full upstream coverage pending |
| `shutil` | Bundled with PS5 fallback | Copy/file operations and temporary-directory cleanup | fd-based `rmtree` hardening is unavailable because `os.scandir(fd)` is `ENOTSUP`; archive and metadata coverage pending |
| `random` | Official wrapper plus native `_random` | Seeded `Random`, choices, and shuffle foundation | System entropy/provider edge cases and full statistical coverage pending |
| `copy` | Official wrapper | Shallow and deep copy of nested data | Custom `__reduce__`/extension-object coverage pending |
| `enum` | Official wrapper | Symbolic values and identity semantics | Full flag, verification, and pickle coverage pending |
| `csv` | Official wrapper plus native `_csv` | Reader/writer round trip and dialect basics | Complete dialect/error/large-file coverage pending |
| `unittest` | Official package bundled | `TestCase`, assertions, and test discovery imports | Full runner, discovery CLI, signal, and isolation coverage pending |
| `subprocess` | Importable patched official wrapper | API import and unsupported-execution behavior | Child execution is unavailable: no `_posixsubprocess`, ELF broker, or reliable descriptor duplication |
| `urllib` | Official `urllib`/`http`/`email` closure bundled | URL parsing, quoting, and `Request` construction | Live HTTP proxy/server, TLS verification, IPv6, and full coverage pending |
| `hashlib` | Official wrapper plus OpenSSL `_hashlib` | MD5, SHA-256, SHA-3, and OpenSSL-backed BLAKE2 | HACL `_blake2`/native extras are not linked; provider edge cases pending |
| `io` | Official wrapper plus native `_io` | `BytesIO` and `StringIO` read/write/seek behavior | File-backed, incremental-codec, nonblocking, and full coverage pending |
| `traceback` | Official wrapper bundled | Exception-only formatting and logging dependency closure | Full chained/stack/source formatting coverage pending |
| `pprint` | Official wrapper bundled | Stable readable formatting of nested structures | Width/recursive/custom-object and full coverage pending |

The dependency closure needed by this tier is intentionally bundled as
official CPython code (`gettext`, `locale`, `encodings.aliases`, `tokenize`,
`inspect`, `email`, `http`, `string`, and `unittest` support modules), rather
than weakening tests when an import exposed a missing dependency.

## `os`

Status: Python-level POSIX wrapper and core filesystem operations included.

Source:

- `upstream/cpython/Lib/os.py`
- `upstream/cpython/Lib/posixpath.py`
- `upstream/cpython/Lib/genericpath.py`
- `upstream/cpython/Lib/stat.py`
- `upstream/cpython/Lib/abc.py`
- `upstream/cpython/Lib/_collections_abc.py`
- native `posix` and `_stat` modules from CPython's static build

Included and tested:

- `os.name`, `os.sep`
- `os.getcwd()`
- `os.listdir()`
- `os.environ` mapping access
- `os.path.join()`
- `os.path.basename()`
- `os.path.dirname()`
- `os.path.abspath()`
- `os.mkdir()` and `os.rmdir()`
- `os.open()`, `os.read()`, `os.write()`, and `os.close()`
- `os.stat()` and file size inspection
- `os.fstat()` and `os.lseek()`
- `os.rename()` and `os.remove()`
- `os.path.exists()` and `os.path.isfile()`
- `os.urandom()` for secure random byte generation through the available
  system entropy source
- `os.getpid()`, `os.getenv()`, `os.putenv()`, and `os.unsetenv()`
- `os.pipe()` and descriptor read/write operations
- `os.fork()`, `os._exit()`, and `os.waitpid()` for a validated immediate-exit
  child process

Not yet covered:

- process creation and management (`exec*`, `spawn*`, `system`)
- subprocess integration
- file-descriptor and terminal helpers
- `os.dup()` and `os.dup2()` return `ENOTSUP` in the current PS5 payload

`fork()` and `waitpid()` are validated by `tests/stdlib/test_process.py`.
`exec*()` and `posix_spawn()` are exposed, but no standard executable path is
available in the current payload filesystem; failed child `execve()` attempts
can leave the payload launcher waiting. `os.system()` is exposed but currently
returns a shell failure status. None of these are yet suitable for a WSGI
process manager.
- extended filesystem metadata and platform-specific permission behavior
- `os.access()` is exposed, but its readability result is not reliable in the
  current PS5 sandbox and is therefore not a passing criterion yet
- `getentropy()` and `getrandom()` are not available in the PS5 build; CPython
  uses its `/dev/urandom` fallback for `os.urandom()`
- complete upstream `test_os.py` regression coverage

The safe in-process POSIX boundary is tested by
`tests/stdlib/test_posix_boundary.py`. Process creation and child management
remain intentionally unverified on PS5 until sandbox permissions and payload
lifetime behavior are established.

## `time`

Status: official CPython native module compiled into the PS5 interpreter.

Source: `upstream/cpython/Modules/timemodule.c`.

Included:

- `time`, `time_ns`
- `gmtime`, `localtime`, `asctime`, `ctime`
- `mktime`, `strftime`, `strptime`
- `clock_gettime`, `clock_gettime_ns`, `clock_getres`
- `clock_settime`, `clock_settime_ns`
- `monotonic`, `monotonic_ns`
- `perf_counter`, `perf_counter_ns`
- `process_time`, `process_time_ns`
- `pthread_getcpuclockid`
- `get_clock_info`
- `sleep` (API present, see limitation below)

Omitted by PS5 build configuration:

- `time.tzset()` because `HAVE_WORKING_TZSET` is not available.
- `thread_time()` and `thread_time_ns()` because `HAVE_THREAD_TIME` is not
  enabled by the PS5 configuration.

Present but limited:

- `time.sleep()` currently reaches the PS5 libc `clock_nanosleep` wrapper,
  which returns `ENOSYS` (`Function not implemented`). The API remains
  exposed, but it is not usable until a native PS5 sleep hook is connected.
- Clock-setting functions may be rejected by platform permissions even though
  they are compiled in.

Tests:

- `tests/stdlib/test_os.py`, adapted from CPython's `Lib/test/test_os.py`.
- `tests/stdlib/test_time.py`, adapted from CPython's `Lib/test/test_time.py`.

## `_signal` and `signal`

Status: the native `signal` module is compiled into the interpreter and safe
signal inspection is available.

Included and tested:

- `signal.SIGINT`
- `signal.getsignal()`

Not yet verified:

- installing handlers for `SIGINT`, `SIGTERM`, or `SIGCHLD`
- signal-driven worker management

Tests:

- `tests/stdlib/test_posix_boundary.py`, adapted from CPython's
  `Lib/test/test_os.py` and `Lib/test/test_signal.py`.

## `io`

Status: Python-level wrapper included over CPython's native `_io` module.

Source:

- `upstream/cpython/Lib/io.py`
- native `_io` module from CPython's static build

Included and tested:

- `io.BytesIO` creation, writing, `getvalue`, `tell`, `seek`, and reading
- `io.StringIO` creation, writing, `getvalue`, `tell`, `seek`, `read`, and
  `readline`

Not yet covered:

- file-backed buffered streams
- incremental codecs and text encoding layers
- asynchronous or non-blocking stream integration
- complete upstream `test_io.py` regression coverage

Tests:

- `tests/stdlib/test_io.py`, adapted from CPython's `Lib/test/test_io.py`.

## `socket` and `select`

Status: native `_socket` and `select` modules are now built statically; the
Python-level `socket` wrapper and basic TCP operations are included.

Included and tested:

- TCP IPv4 socket creation
- `bind`, `listen`, `accept`, and `connect`
- `getsockname` and `getpeername`
- `send` and `recv`
- `setsockopt` with `SO_REUSEADDR` and `TCP_NODELAY`
- numeric and ASCII-hostname `getaddrinfo()` for IPv4 TCP and UDP
- IPv4 UDP `sendto()` and `recvfrom()`

Not yet covered or packaged:

- complete `socket.makefile()` regression coverage
- IPv6 and advanced UDP behavior
- advanced non-blocking mode and `fcntl` flags
- complete upstream `test_socket.py` regression coverage

Platform notes:

- The PS5 static build provides `poll` and `kqueue` capabilities, but the
  Python `selectors` dependency tree is not bundled yet.
- `select.select()` is compiled into the interpreter and verified by
  `tests/stdlib/test_select.py`.
- `select.poll()` and non-blocking IPv4 TCP readiness are verified by
  `tests/stdlib/test_network.py`.
- The PS5 bundle now includes a small `selectors.DefaultSelector` wrapper over
  `select.select()`, with register/modify/unregister, read/write events, and
  context-manager cleanup verified by `tests/stdlib/test_selectors.py`.
- ASCII hostname `getaddrinfo()` and UDP datagrams are covered by
  `tests/stdlib/test_socket.py`. The PS5 bundle uses a minimal IDNA codec for
  ordinary ASCII DNS names; internationalized Unicode domain names remain
  unsupported until full IDNA/Punycode support is added.
- `apps/dns_demo` provides a hardware-facing DNS check. It always resolves
  `localhost`; an external hostname can be selected with
  `CPYTHONPS5_DNS_HOST`, but internet DNS is not a required suite criterion.

Source and tests:

- `upstream/cpython/Lib/socket.py`
- `upstream/cpython/Modules/socketmodule.c`
- `upstream/cpython/Modules/selectmodule.c`
- `tests/stdlib/test_socket.py`
- `tests/stdlib/test_dns.py` (requires working external DNS on PS5)
- `tests/stdlib/test_select.py`
- `tests/stdlib/test_network.py` for `select.poll()` and non-blocking TCP
  readiness behavior

IPv6 remains disabled by the PS5 configure path: enabling it causes CPython's
cross-build probe to reject the SDK `getaddrinfo()` behavior. The current
socket subset is therefore IPv4-only.

## `_ssl`, `_hashlib`, and `hashlib`

Status: statically linked and imported successfully on PS5.

Source:

- OpenSSL 3.5.2, built from the PacBrew `PKGBUILD` configuration
- `upstream/cpython/Modules/_ssl.c`
- `upstream/cpython/Modules/_hashopenssl.c`
- matching `Lib/ssl.py` and `Lib/hashlib.py` wrappers

Included and tested:

- `_ssl` and `_hashlib` static extension modules
- `ssl.OPENSSL_VERSION`
- `ssl.create_default_context()` and TLS 1.2 minimum configuration
- SHA-256 and MD5 through `hashlib` and `_hashlib`

Limitations:

- A live HTTPS handshake is verified on PS5 by
  `tests/stdlib/test_tls_handshake.py`.
- The smoke test uses verification disabled; certificate-store selection and
  certificate verification remain unimplemented. It is intentionally excluded
  from the aggregate suite because it depends on external DNS and internet
  reachability.
- The `hashlib` wrapper omits BLAKE2 because CPython's generated HACL BLAKE2
  objects are not linked; OpenSSL-backed algorithms remain available.

## `base64`, `warnings`, and `_py_warnings`

Status: Python-level modules bundled and tested as dependencies of `ssl`.

Source:

- `upstream/cpython/Lib/base64.py`
- `upstream/cpython/Lib/warnings.py`
- `upstream/cpython/Lib/_py_warnings.py`

Included and tested:

- Base64 encoding and decoding
- Warning filters, capture, and warning emission
- The pure-Python warning implementation fallback

Tests:

- `tests/stdlib/test_ssl_hashlib.py`, adapted from `Lib/test/test_base64.py`
  and `Lib/test/test_warnings.py`.

## `hmac` and `random`

Status: the Python-level `hmac` and `random` wrappers are bundled. Native
`_random` is statically linked; `hmac` uses OpenSSL-backed `_hashlib`.

Included and tested:

- HMAC-SHA256 construction and constant-time comparison through `_hashlib`
- Deterministic seeded `random.Random` output
- OpenSSL-backed SHA-3 and BLAKE2 digest construction through `_hashlib`

The dedicated `_sha3` and `_blake2` HACL modules remain disabled; OpenSSL
provides the tested modern digest implementations instead.

Source and tests:

- `upstream/cpython/Modules/hmacmodule.c`
- `upstream/cpython/Modules/_randommodule.c`
- `upstream/cpython/Lib/hmac.py` and `Lib/random.py`
- `tests/stdlib/test_ssl_hashlib.py`, adapted from `test_hmac.py`,
  `test_random.py`, and `test_hashlib.py`.

## `_thread` and `contextvars`

Status: native `_thread` and `_contextvars` modules are compiled into the PS5
interpreter; the Python-level `contextvars` wrapper is bundled.

Included and tested:

- `_thread.allocate_lock()`
- `_thread.start_joinable_thread()` and `ThreadHandle.join()`
- `_thread.get_ident()`
- `ContextVar` defaults, set/reset tokens, `Context`, and `copy_context()`

The official `threading.py` wrapper is bundled for in-process thread
coordination. Process-oriented concurrency remains platform-limited; see the
Concurrency and parallelism section below.

Source and tests:

- `upstream/cpython/Lib/contextvars.py`
- `upstream/cpython/Modules/_threadmodule.c`
- `upstream/cpython/Modules/_contextvarsmodule.c`
- `tests/stdlib/test_thread_context.py`, adapted from `Lib/test/test_thread.py`
  and `Lib/test/test_contextvars.py`.

## Concurrency and parallelism

Status: the official CPython 3.14.7 thread-pool and multiprocessing wrappers
are bundled where the PS5 payload supports their underlying primitives.

Included and tested on PS5:

- `threading.Thread`, `Event`, `Lock`, thread identities, and joins
- `concurrent.futures.ThreadPoolExecutor`, `Future`, `map()`,
  `as_completed()`, and worker-exception propagation
- `multiprocessing` import, start-method discovery, current-process metadata,
  CPU-count discovery, bidirectional `Pipe`, and `Process` using an explicit
  `fork` context
- native `array`, required by multiprocessing reduction support

Present but unavailable or intentionally omitted:

- `ProcessPoolExecutor` is not bundled; its official `process.py` depends on
  process-launch and subprocess support that the PS5 payload does not provide.
- `multiprocessing.Queue` and `multiprocessing.Semaphore` currently fail with
  `ENOENT` because named semaphores are unavailable in the payload.
- `multiprocessing.shared_memory.SharedMemory` cannot complete resource
  tracking because the `_posixsubprocess` launcher is unavailable; the
  underlying file-backed `mmap` operation is also `ENOTSUP` in this payload.
- `multiprocessing.Process` works with an explicit `fork` context. The default
  `forkserver`/`spawn` paths and cross-process shared state remain unavailable
  without subprocess/ELF-launch support.
- `concurrent.futures._base` now uses the official bundled `logging` package;
  file/network logging handlers remain subject to the filesystem and process
  limitations recorded in the Tier 2 table.

Source and tests:

- `upstream/cpython/Lib/threading.py`, `Lib/queue.py`, and
  `Lib/concurrent/futures/`
- `upstream/cpython/Lib/importlib/` (the small machinery/util closure needed
  by multiprocessing's fork support)
- `upstream/cpython/Lib/multiprocessing/` (with a PS5-only `subprocess` import
  fallback in `tools/patch_multiprocessing_util.py`)
- `upstream/cpython/Modules/_multiprocessing/` and `Modules/arraymodule.c`
- `tests/stdlib/test_concurrency.py`, adapted from `test_threading.py`,
  `test_concurrent_futures.py`, and `_test_multiprocessing.py`

## `csv`, `decimal`, and XML parsing

Status: native parser/arithmetic modules are statically linked and their
Python-level wrappers are bundled.

Included and tested:

- `_csv` and `csv.reader`/`csv.writer`
- `_decimal` and fixed-point arithmetic through `decimal.Decimal`
- `pyexpat` and `_elementtree` through `xml.etree.ElementTree`

Source and tests:

- `upstream/cpython/Modules/_csv.c`
- `upstream/cpython/Modules/_decimal/_decimal.c` and bundled `libmpdec`
- `upstream/cpython/Modules/pyexpat.c` and bundled Expat sources
- `upstream/cpython/Modules/_elementtree.c`
- `upstream/cpython/Lib/csv.py`, `Lib/decimal.py`, and `Lib/xml/etree/`
- `tests/stdlib/test_data_formats.py`, adapted from `test_csv.py`,
  `test_decimal.py`, and `test_xml_etree.py`

Compression modules `zlib`, `_bz2`, and `_lzma` remain disabled because their
external PS5 dependencies are not yet built into this workspace.

## Import runtime, filesystem paths, and temporary files

Status: the native `_stat` module and Python-level `stat`, `posixpath`,
`pathlib`, and `tempfile` wrappers are bundled and tested. The Python-level
`zipimport` wrapper is also bundled over CPython's frozen import machinery.

Included and tested:

- `pathlib.PurePosixPath` composition and inspection
- concrete `pathlib.Path` joins, directory creation, text I/O, iteration, and
  file/directory predicates
- `tempfile.TemporaryDirectory` context-manager cleanup
- `tempfile.NamedTemporaryFile` read/write behavior and automatic unlinking
- `tempfile.mkstemp` creation, descriptor I/O, and explicit cleanup
- `_stat` constants and `stat.S_ISDIR()`/`stat.S_ISREG()` predicates
- `posixpath.join()`
- direct `zipimport` availability

PS5 limitations:

- The PS5 launcher sets `TMPDIR=/user/temp` before CPython initializes.
  `tempfile.gettempdir()` and default temporary-file calls therefore use the
  PS5-managed directory, which is cleaned on restart. Callers may override
  `TMPDIR` or configure `tempfile.tempdir` when needed.
- `shutil.rmtree` uses CPython's path-based fallback because PS5
  `os.scandir(fd)` returns `ENOTSUP`; the fallback still provides automatic
  `TemporaryDirectory` cleanup but does not provide fd-based symlink-race
  hardening.
- Complete upstream `test_pathlib.py` and `test_tempfile.py` coverage remains
  out of scope for the portable PS5 suite.

Source and tests:

- `upstream/cpython/Lib/pathlib/`
- `upstream/cpython/Lib/tempfile.py` and `Lib/shutil.py`
- `upstream/cpython/Lib/zipimport.py`
- `upstream/cpython/Lib/stat.py` and `Lib/posixpath.py`
- `tools/patch_shutil_rmtree.py`
- `tests/stdlib/test_import_runtime.py`, adapted from `test_pathlib.py`,
  `test_zipimport.py`, and `test_stat.py`
- `tests/stdlib/test_filesystem.py`, adapted from `test_pathlib.py` and
  `test_tempfile.py`

## Diagnostics and multiprocessing primitives

Status: native `_tracemalloc`, `_multiprocessing`, and `_posixshmem` are
statically linked. The official CPython `tracemalloc.py` wrapper and the
supported multiprocessing Python wrappers are bundled.

Included and tested:

- `tracemalloc.start()`, `get_traced_memory()`, `stop()`, and `is_tracing()`
- `tracemalloc.take_snapshot()`, `Snapshot.statistics()`, and
  `Snapshot.compare_to()`
- `tracemalloc.Filter`, snapshot filtering, traceback lookup, and
  `Snapshot.dump()`/`Snapshot.load()` on the host
- `_multiprocessing` and `_posixshmem` import availability
- supported `multiprocessing.Pipe` behavior (see the concurrency section)

The full wrapper and dependency bundle pass the PS5 aggregate suite. Full
upstream snapshot/filter coverage, long-running tracing, and cross-process
tracing remain unverified.

Full shared-memory/semaphore behavior remains unavailable in the PS5 sandbox;
the exact Queue, Semaphore, SharedMemory, and process-pool limitations are
recorded in the concurrency section.

The native `mmap` module is compiled and importable, but `mmap.mmap()` returns
`ENOTSUP` in the current PS5 payload. The limitation is covered by
`tests/stdlib/test_diagnostics.py`.

`subprocess` imports through a PS5-patched official wrapper, but execution
remains unavailable: the payload cannot execute ordinary filesystem ELFs
through libc `execve()`, and descriptor duplication is not supported. Process
execution still requires the native kernel-assisted ELF broker described in
the process-management section.

## Collections, algorithms, and dataclasses

Status: native `_collections`, `_heapq`, and `itertools` are statically linked;
the `collections`, `heapq`, and PS5-compatible `dataclasses` wrappers are
bundled.

Included and tested:

- `deque`, `defaultdict`, `Counter`, and `namedtuple`
- `itertools.count()` and `permutations()`
- heap push/pop operations
- dataclass generated initialization, representation, equality, and frozen
  instances

The dataclasses wrapper intentionally covers the foundation subset and does not
yet implement every CPython decorator option or reflection helper.

Tests:

- `tests/stdlib/test_data_structures.py`, adapted from `test_collections.py`,
  `test_itertools.py`, `test_heapq.py`, and `test_dataclasses.py`.

## Profiling and diagnostics wrappers

Status: official CPython `timeit.py`, `dis.py`, `struct.py`, and
`tracemalloc.py` wrappers are bundled. Native `_struct` and `_tracemalloc` are
statically linked.

Included and tested:

- `timeit.timeit()` execution timing
- `timeit.Timer`, repeat timing, callable timing, source validation, and
  garbage-collection suppression during timed sections
- `dis.get_instructions()`, `Bytecode`, code metadata, and formatted output
- `struct.pack()`/`unpack()`, `pack_into()`/`unpack_from()`, `iter_unpack()`,
  layout sizing, and malformed-format errors
- native tracemalloc counters plus snapshots, statistics, filtering,
  comparisons, traceback lookup, and snapshot persistence

The full `tracemalloc.py` wrapper requires and now bundles the small pure-
Python dependency closure for `functools`, `reprlib`, `operator`, `linecache`,
`pickle`, `copyreg`, and `_compat_pickle`.

Missing or not yet verified:

- the complete upstream `test_timeit.py`, `test_dis.py`, `test_struct.py`, and
  `test_tracemalloc.py` suites;
- command-line `timeit`/`dis` entry points and their `argparse` dependency;
- long-running tracing and cross-process/fork tracing on PS5;
- domain-specific allocators and cross-process/fork tracing behavior.

Source:

- `upstream/cpython/Modules/_tracemalloc.c`
- `upstream/cpython/Lib/tracemalloc.py`, `Lib/timeit.py`, `Lib/dis.py`, and
  `Lib/struct.py`

Tests:

- `tests/stdlib/test_profiling.py`, adapted from `test_timeit.py`, `test_dis.py`,
  `test_tracemalloc.py`, and `test_struct.py`.

`_ctypes` is now statically linked against a PS5-built libffi 3.8.0 Unix SysV
backend. The Python wrapper is bundled with a reduced PS5 `sysconfig` surface.
Basic `ctypes.c_int` construction and sizing pass on PS5; loading arbitrary
`.sprx`/`.so` libraries remains unverified.

Source and tests:

- `upstream/cpython/Modules/_tracemalloc.c`
- `upstream/cpython/Modules/_multiprocessing/`
- `tests/stdlib/test_diagnostics.py`, adapted from `test_tracemalloc.py` and
  `test_multiprocessing.py`.

## Core extension modules

Status: statically linked and tested on PS5.

Source:

- `upstream/cpython/Modules/_sre/sre.c`
- `upstream/cpython/Modules/_json.c`
- `upstream/cpython/Modules/_struct.c`
- `upstream/cpython/Modules/mathmodule.c`
- `upstream/cpython/Modules/_datetimemodule.c`
- `upstream/cpython/Modules/_typingmodule.c`
- `upstream/cpython/Modules/_collectionsmodule.c`
- `upstream/cpython/Modules/_functoolsmodule.c`
- `upstream/cpython/Modules/itertoolsmodule.c`
- `upstream/cpython/Modules/_codecsmodule.c`
- `upstream/cpython/Modules/unicodedata.c`
- `upstream/cpython/Modules/arraymodule.c`
- matching `Lib/re/`, `Lib/json/`, `Lib/datetime.py`, `Lib/typing.py`,
  `Lib/annotationlib.py`, `Lib/ast.py`, `Lib/keyword.py`, `Lib/functools.py`,
  and `Lib/codecs.py` wrappers

Included and tested:

- `re` pattern search through static `_sre`
- `json.dumps()` and `json.loads()` through static `_json`
- integer packing and unpacking through static `_struct`
- `math.sqrt()` and floating-point comparison through static `math`
- Unicode character categories through static `unicodedata`
- static `_codecs` availability and existing UTF-8 runtime support
- native `array` is statically linked for multiprocessing reduction and
  descriptor-passing support

Limitations:

- This is focused coverage, not complete upstream regression coverage for
  `re`, `json`, `struct`, `math`, `codecs`, or `unicodedata`.
- The PS5 SDK emits warnings while compiling `mathmodule.c` because its
  type-generic floating-point macros qualify lvalues; the build and runtime
  tests pass.

Tests:

- `tests/stdlib/test_core_modules.py`

## Future module entries

Use this structure for every new module:

```text
## `module_name`

Status: ...
Source: ...
Included and tested: ...
Omitted by build configuration: ...
Present but limited: ...
Tests: ...
```
