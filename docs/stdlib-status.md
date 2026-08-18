# Standard-library implementation status

This file is the source of truth for the PS5 standard-library subset. Every
new module implementation must record:

- the upstream CPython source used;
- APIs included and tested;
- APIs omitted by build-time platform guards;
- APIs present but limited or broken on PS5;
- the corresponding PS5 test file.

The goal is explicit compatibility reporting. A module must not be described
as complete merely because it imports successfully.

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

- A live HTTPS handshake and certificate verification are not yet tested.
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

The higher-level `threading.py` wrapper is not bundled yet because its full
dependency tree and process-oriented APIs are not currently part of the PS5
runtime target.

Source and tests:

- `upstream/cpython/Lib/contextvars.py`
- `upstream/cpython/Modules/_threadmodule.c`
- `upstream/cpython/Modules/_contextvarsmodule.c`
- `tests/stdlib/test_thread_context.py`, adapted from `Lib/test/test_thread.py`
  and `Lib/test/test_contextvars.py`.

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

## Import runtime and paths

Status: the native `_stat` module and Python-level `stat`, `posixpath`, and
`pathlib` support are bundled and tested. The Python-level `zipimport` wrapper
is also bundled over CPython's frozen import machinery.

Included and tested:

- `pathlib.PurePosixPath` path construction and inspection
- `_stat` constants and `stat.S_ISDIR()`/`stat.S_ISREG()` predicates
- `posixpath.join()`
- direct `zipimport` availability

Source and tests:

- `upstream/cpython/Lib/pathlib/`
- `upstream/cpython/Lib/zipimport.py`
- `upstream/cpython/Lib/stat.py` and `Lib/posixpath.py`
- `tests/stdlib/test_import_runtime.py`, adapted from `test_pathlib.py`,
  `test_zipimport.py`, and `test_stat.py`.

## Diagnostics and multiprocessing primitives

Status: native `_tracemalloc`, `_multiprocessing`, and `_posixshmem` are
statically linked and import-tested on PS5.

Included and tested:

- `tracemalloc.start()`, `get_traced_memory()`, `stop()`, and `is_tracing()`
- `_multiprocessing` import availability
- `_posixshmem` import availability

Full shared-memory/semaphore behavior remains unverified in the PS5 sandbox.
The higher-level `multiprocessing` package and process pools are not bundled.

`_ctypes` remains disabled because the first static libffi 3.8.0 attempt selects
its Windows x86-64 `ms_abi` backend under the PS5 cross target, which the PS5
clang rejects. A target-specific libffi backend/configuration is still needed.
Calling native `.sprx` or `.so` APIs therefore still requires a custom C
extension or native launcher bridge.

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
- `upstream/cpython/Modules/_codecsmodule.c`
- `upstream/cpython/Modules/unicodedata.c`
- matching `Lib/re/`, `Lib/json/`, and `Lib/codecs.py` wrappers

Included and tested:

- `re` pattern search through static `_sre`
- `json.dumps()` and `json.loads()` through static `_json`
- integer packing and unpacking through static `_struct`
- `math.sqrt()` and floating-point comparison through static `math`
- Unicode character categories through static `unicodedata`
- static `_codecs` availability and existing UTF-8 runtime support

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
