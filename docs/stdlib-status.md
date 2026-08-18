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

Not yet covered:

- process creation and management (`fork`, `exec*`, `spawn*`, `system`)
- subprocess integration
- file-descriptor and terminal helpers
- extended filesystem metadata and platform-specific permission behavior
- `os.access()` is exposed, but its readability result is not reliable in the
  current PS5 sandbox and is therefore not a passing criterion yet
- `getentropy()` and `getrandom()` are not available in the PS5 build; CPython
  uses its `/dev/urandom` fallback for `os.urandom()`
- complete upstream `test_os.py` regression coverage

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
- non-blocking mode and `fcntl` flags
- complete upstream `test_socket.py` regression coverage

Platform notes:

- The PS5 static build provides `poll` and `kqueue` capabilities, but the
  Python `selectors` dependency tree is not bundled yet.
- `select.select()` is compiled into the interpreter and verified by
  `tests/stdlib/test_select.py`.
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
