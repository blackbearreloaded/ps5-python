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
| `logging` | Official package bundled | Core logger plus official `logging.handlers` and `logging.config` imports, level filtering, `StreamHandler`, and formatter output | Network/file handler behavior, multiprocessing integration, and full upstream coverage pending |
| `shutil` | Bundled with PS5 fallback | Copy/file operations and temporary-directory cleanup | fd-based `rmtree` hardening is unavailable because `os.scandir(fd)` is `ENOTSUP`; archive and metadata coverage pending |
| `random` | Official wrapper plus native `_random` | Seeded `Random`, choices, and shuffle foundation | System entropy/provider edge cases and full statistical coverage pending |
| `copy` | Official wrapper | Shallow and deep copy of nested data | Custom `__reduce__`/extension-object coverage pending |
| `enum` | Official wrapper | Symbolic values and identity semantics | Full flag, verification, and pickle coverage pending |
| `csv` | Official wrapper plus native `_csv` | Reader/writer round trip and dialect basics | Complete dialect/error/large-file coverage pending |
| `unittest` | Official core package bundled | `TestCase`, assertions, and core test-discovery imports | Full runner/discovery CLI, signal/isolation coverage, and `unittest.mock` (which needs advanced dataclass reflection) remain pending |
| `subprocess` | Importable patched official wrapper | API import and unsupported-execution behavior | Child execution is unavailable: no `_posixsubprocess`, ELF broker, or reliable descriptor duplication |
| `urllib` | Official `urllib`/`http`/`email` closure bundled | URL parsing, quoting, and `Request` construction | Live HTTP proxy/server, TLS verification, IPv6, and full coverage pending |
| `hashlib` | Official wrapper plus OpenSSL `_hashlib` | MD5, SHA-256, SHA-3, and OpenSSL-backed BLAKE2 | HACL `_blake2`/native extras are not linked; provider edge cases pending |
| `io` | Official wrapper plus native `_io` | `BytesIO` and `StringIO` read/write/seek behavior | File-backed, incremental-codec, nonblocking, and full coverage pending |
| `traceback` | Official wrapper bundled | Exception-only formatting and logging dependency closure | Full chained/stack/source formatting coverage pending |
| `pprint` | Official wrapper bundled | Stable readable formatting of nested structures | Width/recursive/custom-object and full coverage pending |

The dependency closure needed by this tier is intentionally bundled as
official CPython code (`gettext`, `locale`, `encodings.aliases`, `tokenize`,
`inspect`, `socketserver`, `email`, `http`, `string`, and `unittest` support
modules), rather than weakening tests when an import exposed a missing
dependency.

## Tier 3 concurrency and networking status

The requested Tier 3 modules use the pinned CPython 3.14.7 `Lib/` sources and
the focused `tests/stdlib/test_tier3.py` smoke test, alongside the existing
socket, TLS, select, signal, and concurrency tests.

| Module | PS5 status | Included and tested | Missing or limited |
| --- | --- | --- | --- |
| `asyncio` | Official package bundled | Event loop startup, coroutine/task execution, `asyncio.Queue`, and zero-delay scheduling | Full upstream asyncio suite, subprocess transports, IPv6, and advanced event-loop policy coverage pending |
| `threading` | Official wrapper bundled | Threads, events, locks, identities, joins, and thread-pool integration | Full stress/daemon/interrupt coverage pending |
| `multiprocessing` | Official package bundled | Import, metadata, `Pipe`, and explicit-fork `Process` | Queue/Semaphore/SharedMemory, spawn/forkserver, and cross-process shared state remain unavailable |
| `concurrent.futures` | Official core package bundled | `ThreadPoolExecutor`, futures, mapping, and exception propagation | `ProcessPoolExecutor` cannot launch workers without subprocess/ELF support |
| `socket` | Native `_socket` plus official wrapper | IPv4 TCP/UDP, DNS basics, nonblocking readiness, and socket options | IPv6 is disabled; full makefile/ancillary/upstream coverage pending |
| `ssl` | Native `_ssl` plus official wrapper | OpenSSL context creation and live TLS handshake smoke test | Certificate verification/CA-store selection and full TLS regression coverage pending |
| `http` | Official `http.client`/`http.server` package bundled | Client/server imports and status-code surface | Live server lifecycle, HTTP parsing edge cases, proxy behavior, and full coverage pending |
| `queue` | Official wrapper bundled | Synchronized `Queue` put/get and asyncio queue operations | `PriorityQueue`, `LifoQueue`, shutdown, and full contention coverage pending |
| `select` | Native module built in | `select.select()`, `poll()`, and socket readiness | kqueue integration and complete upstream coverage pending |
| `signal` | Native module built in | Handler installation/restoration and safe signal inspection | Signal-driven worker orchestration and complete upstream coverage pending |

`asyncio`'s official package is shipped together with the recursive `html` and
`mimetypes` dependencies needed by `http.server`. It uses the bundled PS5
`selectors` wrapper over `select.select()`. Child-process asyncio transports
inherit the documented `subprocess`/ELF-launch limitation.

## Tier 4 data structures, algorithms, and formats

These modules use the pinned CPython 3.14.7 `Lib/` sources and the focused
`tests/stdlib/test_tier4_formats.py` smoke test. Native dependencies are built
and statically linked where the upstream module requires them.

| Module | PS5 status | Included and tested | Missing or limited |
| --- | --- | --- | --- |
| `sqlite3` | Official wrapper plus static SQLite 3.46.1 `_sqlite3` | In-memory DB-API connection, schema, inserts, and queries | Loadable extensions, URI/filesystem locking edge cases, and full upstream coverage pending |
| `pickle` | Official wrapper | Nested object round trip | Cross-version protocol stress and complete regression coverage pending |
| `struct` | Official wrapper plus native `_struct` | Network-order packing/unpacking | Full format/error coverage pending |
| `bisect` | Official wrapper | Sorted insertion | Key-function and full regression coverage pending |
| `heapq` | Official wrapper plus native `_heapq` | Heap push/pop priority ordering | Max-heap and full regression coverage pending |
| `array` | Native static module | Integer array creation, append, and conversion | Type-code and buffer-protocol edge coverage pending |
| `operator` | Native static module plus official wrapper | Callable arithmetic operation | Full operator and attr/item helper coverage pending |
| `decimal` | Official wrapper plus native `_decimal` | Fixed-point arithmetic | Context, signal, and full arithmetic coverage pending |
| `fractions` | Official wrapper | Rational reduction and arithmetic | Decimal/string conversion and full regression coverage pending |
| `zlib` | Static zlib 1.3.1 plus native `zlib` module | Compress/decompress and CRC32 | Streaming/error stress and full upstream coverage pending |
| `gzip` | Official wrapper over zlib | File-object compression round trip | Multi-member, metadata, and full coverage pending |
| `zipfile` | Official package over zlib | In-memory ZIP write/read, including cp437 metadata | Encryption, path traversal policy, large archives, and full coverage pending |
| `tarfile` | Official wrapper over zlib | In-memory TAR write/read | Full filter/security, sparse, and compression-mode coverage pending |
| `base64` | Official wrapper | Base64 encode/decode | Full Base16/Base32/Base85/error coverage pending |
| `xml` | Official `xml.etree`, `xml.dom`, and `xml.sax` wrappers plus native Expat/ElementTree | ElementTree, minidom, and SAX parsing | External entities, validation, and full parser regression coverage pending |
| `tempfile` | Official wrapper using `/user/temp` | Temporary directory/file behavior and cleanup | fd-hardening and full upstream coverage pending |
| `glob` | Official wrapper | Wildcard expansion in the PS5 temporary directory | Recursive hidden-file and dirfd behavior pending |
| `fnmatch` | Official wrapper | Shell-style filename matching | Case-normalization and full regression coverage pending |

Static dependencies are reproducibly built by `tools/build_zlib_ps5.sh`,
`tools/build_sqlite3_ps5.sh`, `tools/build_bzip2_ps5.sh`, and
`tools/build_xz_ps5.sh`.

## Tier 5 metaprogramming, inspection, and execution status

These modules use the pinned CPython 3.14.7 `Lib/` sources and focused Tier 5
tests. PS5 startup and source-layout limitations are recorded explicitly.

| Module | PS5 status | Included and tested | Missing or limited |
| --- | --- | --- | --- |
| `inspect` | Official wrapper | Predicates, signatures, argument binding, members, frames, and source lookup | Source recovery is unavailable for launcher-executed in-memory scripts; full upstream coverage pending |
| `ast` | Official wrapper plus native `_ast` | Parsing, literal evaluation, tree walking, transformations, locations, dump, and unparse | Full compiler/error-location and regression coverage pending |
| `dis` | Official wrapper plus native bytecode metadata | Instruction iteration, `Bytecode`, code info, and disassembly | Version-specific opcode and complete regression coverage pending |
| `importlib` | Official package subset, including `importlib.resources` and `importlib.metadata` | `import_module`, `find_spec`, machinery finders/loaders, relative resolution, ABCs, resource traversal used by `zoneinfo`, and framework version fallback/discovery APIs | Full meta-path, zip/import hook, cache invalidation, and distribution metadata deployment coverage pending |
| `abc` | Official wrapper plus native `_abc` | Abstract methods, registration, virtual subclasses, and instance checks | Full cache-token and registry stress coverage pending |
| `contextlib` | Official wrapper | `nullcontext`, `contextmanager`, `ExitStack`, and `suppress` | Async/context decorator edge cases and full coverage pending |
| `gc` | Native built-in | Enable/disable state and explicit collection | PS5 allocator tuning and debug hooks remain unverified |
| `site` | Official wrapper bundled | Prefix/user-site queries and startup-safe import | Automatic site customization is intentionally disabled by the PS5 launcher |
| `sysconfig` | Official package with PS5 static-build fallback | Schemes, paths, platform, Python version, and config variables | Complete generated build-variable parity and installation-layout coverage pending |
| `weakref` | Official wrapper plus native weakref support | Weak-reference lifecycle and collection | Proxy/finalizer stress and full regression coverage pending |
| `codecs` | Official wrapper plus native `_codecs` | Registry lookup and UTF-8 transforms | Full codec alias/error-handler and incremental-stream coverage pending |
| `types` | Official wrapper plus native built-in types | `SimpleNamespace`, mapping proxies, and dynamic function types | Full dynamic-class/descriptor coverage pending |

The focused tests are `tests/stdlib/test_tier5_inspection.py`,
`test_tier5_import.py`, and `test_tier5_runtime.py`; their upstream mapping is
recorded in `tests/UPSTREAM_TESTS.md`.

## Tier 6 security, internationalization, text, and POSIX status

These modules use the pinned CPython 3.14.7 `Lib/` sources and focused Tier 6
tests. Windows-only modules (`msvcrt`, `winreg`, and `winsound`) are excluded
from this PS5 target as requested.

| Module | PS5 status | Included and tested | Missing or limited |
| --- | --- | --- | --- |
| `secrets` | Official wrapper over OS/OpenSSL randomness | Tokens, `randbelow`, and constant-time comparisons | Full entropy-source and statistical regression coverage pending |
| `hmac` | Official wrapper over OpenSSL-backed hashlib | SHA-256 MACs and digest comparison | Full digestmod/provider and streaming coverage pending |
| `getpass` | Official wrapper | Non-interactive hidden-input helper and validation | Interactive terminal prompting is limited by PS5 console behavior |
| `gettext` | Official wrapper | Null translations, plural forms, and contextual lookup | Catalog loading and locale discovery coverage pending |
| `locale` | Official wrapper | C locale, numeric formatting, conventions, and encoding query | Full locale database/alias coverage pending |
| `unicodedata` | Native Unicode database module | Names, categories, lookup, and normalization | Complete Unicode-version regression coverage pending |
| `string` | Official package | `Template` and `Formatter` behavior | Full constants/format grammar coverage pending |
| `textwrap` | Official wrapper | Wrapping, dedent, and indentation | Full whitespace and sentence-ending coverage pending |
| `difflib` | Official wrapper | Sequence opcodes and unified diffs | Full matcher/autojunk and rendering coverage pending |
| `mimetypes` | Official wrapper | Common MIME guessing and custom type registration | Complete system-database and platform mapping coverage pending |
| `uuid` | Official wrapper plus recursive `platform` dependency | UUID parsing, UUID4/UUID5 generation, bytes, URN, and RFC 4122 variant | OS-specific node/MAC discovery and full entropy/provider coverage pending |
| `stat` | Official wrapper plus native stat support | Mode predicates and `filemode()` | Full platform-specific flags and formatting coverage pending |
| `filecmp` | Official wrapper | Shallow/deep file comparison and cache behavior using `/user/temp` | Directory comparison and symlink/metadata edge coverage pending |
| `termios` | Native static module | PS5 termios constants and tty mode transformations | Real TTY device control and full ioctl coverage pending |
| `tty` | Official wrapper | `cfmakeraw` and `cfmakecbreak` transformations | Interactive terminal state transitions pending |
| `fcntl` | Native static module | Descriptor flags and `FD_CLOEXEC` through `fcntl()` | Full lock/ioctl/owner coverage pending |
| `resource` | Native static module | `getrusage()`, `getrlimit()`, and `setrlimit()` | Full limit catalog and accounting coverage pending |

The focused tests are `test_tier6_security_i18n.py`,
`test_tier6_text_formats.py`, and `test_tier6_posix.py`; their upstream
CPython test sources are mapped in `tests/UPSTREAM_TESTS.md`.

## Tier 7 developer tools, debugging, and profiling status

These modules use the pinned CPython 3.14.7 `Lib/` sources and focused Tier 7
tests. The PS5 payload supports deterministic in-process tooling; interactive
terminal and subprocess-dependent portions remain explicitly limited.

| Module | PS5 status | Included and tested | Missing or limited |
| --- | --- | --- | --- |
| `pdb` | Official wrapper with `bdb`/`cmd` dependencies | Debugger construction, prompt, and path canonicalization | Interactive stepping and terminal session coverage pending |
| `timeit` | Official wrapper | Callable/source timing, repeats, validation, and GC handling | Full benchmark/statistical regression coverage pending |
| `cProfile` | Official wrapper plus native `_lsprof` | `Profile`, `runcall`, context-manager profiling, and call statistics | Long-running/forked profiling and full upstream coverage pending |
| `profile` | Official pure-Python profiler | Call collection, `runcall`, and pstats-compatible output | Full recursion/dispatch and performance coverage pending |
| `pstats` | Official wrapper | Sorting, reports, and structured profile summaries | Complete input-format and browser/CLI coverage pending |
| `tracemalloc` | Official wrapper plus native `_tracemalloc` | Counters, snapshots, statistics, filtering, comparison, traceback, and persistence | Long-running and cross-process tracing remain unverified |
| `doctest` | Official wrapper | Parser, runner, and passing interactive example | Module discovery, CLI, and full traceback/output coverage pending |
| `py_compile` | Official wrapper | Direct source-to-bytecode compilation | Full invalid-source, permissions, and cache-tag coverage pending |
| `compileall` | Official wrapper | Single-file and recursive directory compilation | CLI, symlink, worker, and full filesystem coverage pending |
| `code` | Official wrapper | `InteractiveInterpreter` and `InteractiveConsole` command execution | Interactive terminal loop and error-display coverage pending |
| `codeop` | Official wrapper | Incomplete/complete/syntax-error command compilation | Full compiler flag and interactive buffering coverage pending |
| `readline` | PS5-safe compatibility layer | History, completion hooks, line-buffer state, and file persistence APIs | GNU/editline native line editing is unavailable because no linkable backend is shipped |
| `rlcompleter` | Official wrapper over readline-compatible APIs | Namespace completion behavior | Full interactive completion integration pending |

The focused tests are `tests/stdlib/test_tier7_compile.py` and
`test_tier7_interactive.py`; profiler coverage extends
`tests/stdlib/test_profiling.py`. Their upstream mappings are recorded in
`tests/UPSTREAM_TESTS.md`.

## Tier 8 feasible utility status

These are the feasible, non-desktop portions of Tier 8, using the pinned
CPython 3.14.7 `Lib/` sources and the focused
`tests/stdlib/test_tier8_pure.py` test. Windows-only modules and GUI/terminal
database stacks are tracked as intentionally omitted.

| Module | PS5 status | Included and tested | Missing or limited |
| --- | --- | --- | --- |
| `graphlib` | Official wrapper bundled | `TopologicalSorter` ordering, completion, and cycle detection | Full parallel-worker and regression coverage pending |
| `statistics` | Official wrapper bundled | Mean, median, mode, variance, and exact `Fraction` inputs | Full numeric-type/error and regression coverage pending |
| `cmath` | Native static module | Complex square root, exponential, polar/rectangular conversion | Full platform floating-point and regression coverage pending |
| `ipaddress` | Official wrapper bundled | IPv4/IPv6 parsing, membership, interfaces, and range summarization | Full formatting/error and regression coverage pending |
| `colorsys` | Official wrapper bundled | RGB/HLS/HSV conversion round trips | Full conversion edge coverage pending |
| `calendar` | Official wrapper bundled | Leap years, month ranges, and month iteration | Locale/CLI rendering and full coverage pending |
| `zoneinfo` | Official package plus native `_zoneinfo` | Package import and named-zone lookup when PS5 tzdata is installed | No timezone database is shipped in the payload; named zones may raise `ZoneInfoNotFoundError` |
| `wave` | Official wrapper bundled | In-memory PCM WAV write/read round trip | Compressed/extended chunks and full coverage pending |
| `binascii` | Native static module | Hex conversion and CRC32 | Full ASCII/base64/quoted-printable error coverage pending |

`test_tier8_pure.py` is adapted from the corresponding CPython tests listed in
`tests/UPSTREAM_TESTS.md`.

## Tier 8 compression and persistence status

| Module | PS5 status | Included and tested | Missing or limited |
| --- | --- | --- | --- |
| `bz2` | Static bzip2 1.0.8 plus native `_bz2` | Bytes round trip | Full streaming, corruption, and upstream regression coverage pending |
| `lzma` | Static xz/liblzma 5.6.3 plus native `_lzma` | Bytes round trip | Full filters, containers, and upstream regression coverage pending |
| `shelve` | Official wrapper bundled | Shelf round trip over `dbm.dumb` | Native dbm-backed locking and full regression coverage pending |
| `dbm` | Pure package with `dbm.dumb` | Key/value round trip | GDBM/NDBM and other native database backends are unavailable |

`tkinter` and `curses` remain omitted because no Tcl/Tk or curses backend is
shipped. `smtpd` is absent from CPython 3.14.7 itself. Desktop UI and native
database backends remain outside the feasible PS5 subset.

## Tier 8 protocol and mail status

These pure-Python clients and mailbox helpers use the pinned CPython 3.14.7
`Lib/` sources. The focused `tests/stdlib/test_tier8_protocols.py` uses only
loopback fake servers and local mailbox files; it never contacts external
services.

| Module | PS5 status | Included and tested | Missing or limited |
| --- | --- | --- | --- |
| `ftplib` | Official wrapper bundled | Local FTP greeting/login/PWD exchange | TLS, active/passive data transfers, and full upstream coverage pending |
| `poplib` | Official wrapper bundled | Local POP3 greeting and QUIT exchange | Authentication, message retrieval, TLS, and full upstream coverage pending |
| `imaplib` | Official wrapper bundled | Local IMAP4 CAPABILITY/LOGIN/LOGOUT exchange | TLS, mailbox state/search/fetch, and full upstream coverage pending |
| `smtplib` | Official wrapper bundled | Local SMTP EHLO/MAIL/RCPT/DATA exchange with `EmailMessage` | TLS/authentication, delivery semantics, and full upstream coverage pending |
| `mailbox` | Official wrapper bundled | Maildir add/read round trip under `/user/temp` on PS5 | mbox locking, alternate mailbox formats, and full upstream coverage pending |
| `email` | Complete official package tree bundled | Message parsing, MIME text, and SMTP serialization dependencies | Full parser/policy/attachment regression coverage pending |
| `smtpd` | Unavailable in CPython 3.14.7 | Not present in the pinned upstream `Lib/` | Removed upstream; use an application-level SMTP server implementation when one is needed |

The complete recursive `email` tree, including `email.mime`, is shipped so
`smtplib` and `mailbox` do not depend on host-installed modules. Network
clients are importable and loopback-tested, but PS5 has no external-service
or daemon lifecycle guarantee. Windows-only behavior is outside this PS5
target.

## Tier 9 core and interpreter modules

These modules are either provided by the CPython interpreter itself or are
small official Python wrappers. The focused
`tests/stdlib/test_tier9_core.py` test is adapted from CPython 3.14.7's
`test_future_stmt`, `test_marshal`, `test_copyreg`, and `test_thread` tests.

| Module | PS5 status | Included and tested | Missing or limited |
| --- | --- | --- | --- |
| `__main__` | Interpreter-provided | Top-level module identity and execution namespace | Launcher-specific `-m`/script startup permutations remain pending |
| `__future__` | Official wrapper bundled | Feature names, release metadata, compiler flags, and future annotations | Full syntax-error and interactive REPL matrix pending |
| `builtins` | Interpreter-provided | Built-in lookup, functions, and exception identity | Complete builtins regression coverage pending |
| `_thread` | Native static module | Locks, non-blocking acquire, thread identity, joinable thread startup, and join | Full stack-size, interrupt, shutdown, and stress coverage pending |
| `marshal` | Native interpreter module | Scalar/container/code-object round trips and `allow_code=False` | Full format/version, malformed-input, and compatibility coverage pending |
| `copyreg` | Official wrapper bundled | Custom reduction registration and private slot-name handling | Full extension-registry and pickle compatibility coverage pending |

`__main__`, `builtins`, and `marshal` do not require runtime files: they are
provided by the statically linked interpreter. `_thread` is likewise compiled
into the executable. `__future__.py` and `copyreg.py` are copied from the
pinned upstream `Lib/` tree into the runtime bundle; no host modules are used.

## Tier 9 legacy and low-level utility status

The feasible Tier 9 wrappers use the pinned CPython 3.14.7 `Lib/` sources and
the focused `tests/stdlib/test_tier9_legacy.py` checks. Terminal, browser, and
GUI actions are represented only up to the point supported by the PS5
payload; the implementation does not claim a desktop environment exists.

| Module | PS5 status | Included and tested | Missing or limited |
| --- | --- | --- | --- |
| `cmd` | Official wrapper bundled | Command parsing, dispatch, and command result handling | Interactive terminal loop and completion integration pending |
| `shlex` | Official wrapper bundled | POSIX splitting, quoting, and joining | Full shell grammar/error regression coverage pending |
| `optparse` | Official wrapper bundled | Option registration, boolean/value parsing, and positional arguments | Complete deprecated API and formatter coverage pending |
| `getopt` | Official wrapper bundled | Short/long option parsing and option errors | Complete GNU permutation and CLI coverage pending |
| `pydoc` | Official wrapper plus `pydoc_data`/pager dependencies | Object lookup and plain-text rendering | Interactive pager, HTTP documentation server, and full module discovery coverage pending |
| `webbrowser` | Official wrapper bundled | Browser registration and controller selection | No PS5 desktop browser process; launching a browser is unavailable |
| `symtable` | Official wrapper over native `_symtable` | Module/function symbol table creation and parameter lookup | Complete compiler-scope and type-parameter regression coverage pending |
| `turtle` | Omitted | Not shipped | Requires `tkinter`/Tcl/Tk, which are unavailable on PS5 |

`__main__`, `__future__`, `builtins`, `_thread`, `marshal`, and `copyreg` are
covered by the Tier 9 core section above. The complete `pydoc_data` package and
`_pyrepl.pager` are bundled so `pydoc` does not fall back to host-installed
files. `webbrowser` remains useful for controller registration and URL
construction, but its OS browser backends cannot launch on the headless PS5.

## WSGI application boundary

The official `wsgiref` package is the first supported WSGI server target. The
focused `tests/stdlib/test_wsgi.py` check is adapted from CPython 3.14.7's
`test_wsgiref.py`: it runs a real `wsgiref.simple_server` on loopback, sends
an HTTP request through `http.client`, and verifies the WSGI environment,
response headers, body, and validation middleware. The companion
`test_wsgi_threaded.py` check exercises a blocked request, a concurrent
request, and controlled server shutdown using `ThreadingMixIn`.

| Module | PS5 status | Included and tested | Missing or limited |
| --- | --- | --- | --- |
| `wsgiref` | Official package bundled | Single-process and threaded loopback request/response, concurrency, and shutdown | Full CGI, signal, malformed-request, keep-alive, and high-throughput coverage pending |
| `gunicorn` 23.0.0 | Vendored pure-Python package bundled | Official sync worker and HTTP parser; pre-fork master/worker; normal TCP bind and inherited `fd://` listener; loopback WSGI request; SIGTERM/SIGCHLD shutdown and reaping | Daemon/re-exec, Unix sockets, distribution-metadata plugin entry points, and optional gevent/eventlet workers are outside the PS5 contract |

### Flask/Werkzeug application boundary

The pinned pure-Python framework closure is bundled for CPython **3.14.7**:

| Package | Version | PS5 status |
| --- | --- | --- |
| Flask | 3.1.3 | Bundled; routing, JSON responses, Jinja rendering, signed sessions, and Gunicorn WSGI serving pass |
| Werkzeug | 3.1.8 | Bundled; WSGI test client and Flask request/response boundary pass |
| Jinja2 / MarkupSafe | 3.1.6 / 3.0.3 | Bundled; pure-Python template rendering and HTML escaping pass; native speedups omitted |
| ItsDangerous / Click / Blinker | 2.2.0 / 8.2.1 / 1.9.0 | Bundled; session signing and Flask dependency imports pass |

The focused `tests/stdlib/test_flask.py` check is adapted from CPython's
WSGI, cookie, and cookie-jar tests. It exercises Flask's test client,
Werkzeug's WSGI client, JSON request/response handling, Jinja escaping, a
signed session cookie, and a real Flask app served by Gunicorn's sync worker.
The Flask development reloader/debugger, dotenv discovery, multi-process
serving, and optional async workers remain disabled on PS5.

This establishes the application/server interface needed by Flask-style WSGI
applications and validates both the reference threaded server and Gunicorn's
normal synchronous pre-fork path. The package does not silently enable
daemonization, re-exec, Unix-domain sockets, plugin entry points, or
third-party async workers on this target. See
[`docs/gunicorn-foundation.md`](gunicorn-foundation.md) for the lifecycle
contract and remaining bounded-test work.

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
  Python `selectors` wrapper is intentionally a small `select.select()`-
  backed implementation used by the bundled asyncio package.
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

The static zlib 1.3.1, bzip2 1.0.8, and xz/liblzma 5.6.3 dependencies and
their native modules are bundled and round-trip tested. Full upstream
compression coverage remains pending.

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

Status: official CPython `timeit.py`, `dis.py`, `struct.py`, `tracemalloc.py`,
`cProfile.py`, `profile.py`, and `pstats.py` wrappers are bundled. Native
`_struct`, `_tracemalloc`, and `_lsprof` are statically linked.

Included and tested:

- `timeit.timeit()` execution timing
- `timeit.Timer`, repeat timing, callable timing, source validation, and
  garbage-collection suppression during timed sections
- `dis.get_instructions()`, `Bytecode`, code metadata, and formatted output
- `struct.pack()`/`unpack()`, `pack_into()`/`unpack_from()`, `iter_unpack()`,
  layout sizing, and malformed-format errors
- native tracemalloc counters plus snapshots, statistics, filtering,
  comparisons, traceback lookup, and snapshot persistence
- `cProfile.Profile` and pure-Python `profile.Profile` call collection,
  `runcall()`, context-manager profiling, and pstats-compatible reports
- `pstats.Stats` sorting, text reports, and structured
  `get_stats_profile()` summaries

The full `tracemalloc.py` wrapper requires and now bundles the small pure-
Python dependency closure for `functools`, `reprlib`, `operator`, `linecache`,
`pickle`, `copyreg`, and `_compat_pickle`.

Missing or not yet verified:

- the complete upstream `test_timeit.py`, `test_dis.py`, `test_struct.py`, and
  `test_tracemalloc.py` suites;
- the complete upstream `test_profile.py`, `test_cprofile.py`, and
  `test_pstats.py` suites;
- command-line `timeit`/`dis` entry points and their `argparse` dependency;
- long-running tracing and cross-process/fork tracing on PS5;
- domain-specific allocators and cross-process/fork tracing behavior.

The profilers are available for normal in-process execution.  PS5 validation
currently covers deterministic call collection and report generation; profiler
output involving forked workers, interactive terminals, or very long-running
sessions remains unverified.

Source:

- `upstream/cpython/Modules/_tracemalloc.c`
- `upstream/cpython/Modules/_lsprof.c`, `Modules/rotatingtree.c`
- `upstream/cpython/Lib/tracemalloc.py`, `Lib/timeit.py`, `Lib/dis.py`,
  `Lib/struct.py`, `Lib/cProfile.py`, `Lib/profile.py`, and `Lib/pstats.py`

Tests:

- `tests/stdlib/test_profiling.py`, adapted from `test_timeit.py`, `test_dis.py`,
  `test_tracemalloc.py`, `test_struct.py`, `test_profile.py`,
  `test_cprofile.py`, and `test_pstats.py`.

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
