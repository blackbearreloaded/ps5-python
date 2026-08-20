# CPythonPS5

Experimental port of the CPython language runtime to the jailbroken PS5 payload environment.

This repository currently targets **CPython 3.14.7**, pinned to upstream source
commit `823f0323ee6ec1402088b73bce1a38473cac36dc`. The exact pin is recorded in
[CPYTHON_VERSION.txt](CPYTHON_VERSION.txt); this is not an unpinned “latest
Python” port.

The first target is deliberately small: execute Python language semantics without depending on the Python standard library or third-party modules. The runtime will eventually be embedded in a native PS5 application, but this repository begins with a standalone interpreter proof of concept.

## Scope of the first milestone

- Cross-compile a released CPython version with the PS5 SDK.
- Start the standalone CPython ELF directly.
- Execute a script supplied as a separate file, for example `main.py`.
- Validate expressions, functions, classes, exceptions, generators, comprehensions, and garbage collection.

The first milestone did not include the standard-library package tree,
networking, SQLite, `pip`, native extension loading, GUI support, or subprocess
APIs. The current work selectively bundles tested CPython 3.14.7 standard
library modules while retaining those broader exclusions. The interpreter
still contains CPython’s required built-in runtime modules; the test script
itself must remain import-free. Initial hardware testing uses one ELF plus an
external `/data/python/main.py` file.

## References

- [CPython](https://github.com/python/cpython)
- [MicroPython](https://github.com/micropython/micropython)
- [PS5 PacBrew packages](https://github.com/ps5-payload-dev/pacbrew-repo)

See [PLAN.md](PLAN.md) for the implementation roadmap and acceptance criteria.
The Web Launcher product roadmap is tracked separately in
[roadmap.md](roadmap.md).
The language-core test provenance and portable-subset rules are documented in
[`tests/UPSTREAM_TESTS.md`](tests/UPSTREAM_TESTS.md).

## Status

Core scaffold started. The pinned CPython checkout is kept outside Git under
`upstream/cpython`; run `make source-fetch` to recreate it. The host-side
language-core suite and its PS5 aggregate run pass. The PS5 core ELF and
static library build with WSL and the installed PS5 SDK.

The current CPython 3.14.7 runtime bundle includes the official
`threading.py`, `concurrent.futures.ThreadPoolExecutor`, and supported
`multiprocessing` wrappers. Thread pools and `multiprocessing.Pipe` are
verified on PS5; Queue/Semaphore, POSIX `SharedMemory`, and
`ProcessPoolExecutor` remain unavailable because the standard subprocess
launch path is still unsupported. The web launcher now has a separate,
project-specific app supervisor for packaged applications; it does not change
the general stdlib subprocess contract. See
[`docs/stdlib-status.md`](docs/stdlib-status.md) for the per-module contract.
The same bundle includes `pathlib.Path` and `tempfile`; the PS5 launcher
defaults temporary files to the managed `/user/temp` directory, which is
cleaned on restart.
It also includes the official `typing.py` dependency closure (`annotationlib`,
`ast`, and `keyword`) and `datetime.py` over native `_typing` and `_datetime`;
the timezone database remains outside the current subset.
Tier 2 utility modules are also bundled from the same CPython 3.14.7 pin,
including `argparse`, `logging`, `shutil`, `random`, `copy`, `enum`, `csv`,
`unittest`, `subprocess` (import-only on PS5), `urllib`, `hashlib`, `io`,
`traceback`, and `pprint`; their individual omissions are tracked in
[`docs/stdlib-status.md`](docs/stdlib-status.md).
Tier 3 concurrency and networking modules are now included as well: official
`asyncio`, `threading`, `multiprocessing`, and `concurrent.futures` wrappers,
plus the native-backed `socket`, `ssl`, `http`, `queue`, `select`, and `signal`
surfaces. Async event loops and IPv4 socket readiness are verified on PS5;
process pools, child-process transports, IPv6, and certificate verification
remain platform-limited as documented.
Tier 4 data structures and formats are also included: static SQLite 3.46.1,
static zlib 1.3.1, `pickle`, `struct`, `bisect`, `heapq`, `array`, `operator`,
`decimal`, `fractions`, `zlib`, `gzip`, `bz2`, `lzma`, `zipfile`, `tarfile`,
`base64`, XML, `tempfile`, `glob`, and `fnmatch`. bzip2 1.0.8 and xz/liblzma
5.6.3 are statically linked for the tested compression subset.
Tier 5 metaprogramming and inspection modules are included as well: `inspect`,
`ast`, `dis`, `importlib`, `abc`, `contextlib`, `gc`, `site`, `sysconfig`,
`weakref`, `codecs`, and `types`. Their PS5-specific startup and source-layout
limits are recorded in the standard-library status report.
Tier 6 security, text, and POSIX utilities are included too: `secrets`, `hmac`,
`getpass`, `gettext`, `locale`, `unicodedata`, `string`, `textwrap`, `difflib`,
`mimetypes`, `uuid`, `stat`, `filecmp`, `termios`, `tty`, `fcntl`, and
`resource`. Windows-only modules are intentionally excluded.
Tier 7 developer tools are included as well: `pdb`, `timeit`, `cProfile`,
`profile`, `pstats`, `tracemalloc`, `doctest`, `py_compile`, `compileall`,
`code`, `codeop`, `readline`, and `rlcompleter`. PS5 uses a compatibility
readline layer without GNU/editline native line editing.
Tier 8's feasible utility subset is included too: `graphlib`, `statistics`,
`cmath`, `ipaddress`, `colorsys`, `calendar`, `zoneinfo`, `wave`, `binascii`,
`ftplib`, `poplib`, `imaplib`, `smtplib`, `mailbox`, `email`, `shelve`, and
pure `dbm.dumb`. Named timezone data is not bundled; `tkinter`, `curses`,
native dbm backends, `smtpd`, Windows-only modules, and desktop terminal
integrations remain outside the PS5 target.
Tier 9 core and legacy utilities are included where feasible: `__future__`,
`builtins`, `_thread`, `marshal`, `copyreg`, `cmd`, `shlex`, `optparse`,
`getopt`, `pydoc`, `webbrowser`, and `symtable`. `turtle` remains omitted
because it requires the unavailable Tcl/Tk GUI stack; browser and interactive
terminal launch paths are documented as headless-PS5 limitations.
The official CPython 3.14.7 `wsgiref` reference server is also bundled.
Single-process and threaded loopback WSGI requests pass on PS5. Separately,
the project vendors a constrained third-party web stack: Gunicorn 23.0.0 and
the Flask 3.1.3 closure (Werkzeug 3.1.8, Jinja2 3.1.6, MarkupSafe 3.0.3,
ItsDangerous 2.2.0, Click 8.2.1, and Blinker 1.9.0). Their sync serving,
JSON, Jinja escaping, signed sessions, and Werkzeug WSGI client pass on PS5.
Daemon/re-exec, Unix-domain sockets, plugin entry points, debug/reloader
subprocesses, and optional gevent/eventlet workers remain outside the PS5
contract. See [`docs/web-stack-status.md`](docs/web-stack-status.md).

The browser web launcher deploys a small `python-app-supervisor.elf` alongside
`python-web.elf`. Packaged applications run as forked child processes with
separate PIDs and targeted Stop control, while the persistent interpreter and
script editor remain in the web process. See
[`docs/web-launcher.md`](docs/web-launcher.md).

## Build the first PS5 ELF

Run these from WSL at the repository root:

```sh
make source-fetch
make host-build
make ps5-core
```

The PS5 build and test targets invoke `lint` automatically. Run the source
quality checks directly when iterating from WSL:

```sh
make format       # apply the checked-in clang-format policy
make format-check # verify formatting without modifying files
make tidy         # run clang-tidy with the PS5 compile definitions
make lint         # run format-check and tidy
```

The project-owned C and header files under `src/` are formatted using the
checked-in [`.clang-format`](.clang-format) policy. Clang-Tidy uses
[`.clang-tidy`](.clang-tidy) and analyzes those files with C11 and the same
PS5-facing include paths used by the launcher build. Vendored CPython sources
are intentionally excluded.

PS5 builds use all WSL CPU threads by default and automatically use `ccache`
when available. Select the compiler cache explicitly with `PS5_CACHE=ccache`
or `PS5_CACHE=sccache`; disable caching with `PS5_CACHE=none` (or the legacy
`PS5_CCACHE=0`). The cache applies to both CPython compilation and launcher
link commands.

The SDK uses LLVM `lld` by default. `mold` is supported as an opt-in
experiment:

```sh
PS5_LINKER=mold make ps5-core
```

Use `PS5_JOBS=8` to override automatic parallelism. `sccache` must be
installed inside WSL and available on its `PATH`; the same applies to `ccache`
and `mold`.

The core artifacts are:

- `build/ps5/python.elf` — the standalone PS5 interpreter launcher.
- `build/ps5/libpython3.14.a` — the static runtime library for a future custom launcher.
- `build/ps5/cpython-lib/` — the minimal external runtime bundle, including
  the Python-level `os` path wrappers.

`time` is already compiled into the static interpreter as a built-in C module.
The initial standard-library slice adds `os` and its POSIX path/stat/ABC
dependencies from the matching CPython checkout. The hardware test is
`tests/stdlib/os_time.py`.

The launcher accepts the script path as its first argument and an optional
runtime directory as its second argument. If omitted, the runtime directory
defaults to `/data/python/cpython-lib`:

```text
python.elf /data/python/main.py /data/python/cpython-lib
```

The reproducible hardware loop builds the ELF and runtime bundle, creates the
required PS5 FTP directories, uploads them, and launches the ELF:

```sh
PS5_HOST=192.168.4.30 make ps5-run
PS5_HOST=192.168.4.30 make ps5-run SCRIPT=examples/main.py
PS5_HOST=192.168.4.30 make ps5-run SCRIPT=tests/core_suite.py
```

The target is not a native app package yet; it is intentionally one ELF, one
external Python script, and one small runtime directory.

Phase 4 lifetime checks are available with:

```sh
make host-lifetime
PS5_HOST=192.168.4.30 make ps5-lifetime
```

Run the aggregate hardware tests, including the standard-library checks, with:

```sh
PS5_HOST=192.168.4.30 make ps5-test
```

Run those tests plus the repeated-process lifetime checks with:

```sh
PS5_HOST=192.168.4.30 make ps5-suite
```

The live TLS smoke test can also be run directly when external DNS and internet
access are available:

```sh
PS5_HOST=192.168.4.30 make ps5-run SCRIPT=tests/stdlib/test_tls_handshake.py
```

`ps5-core` and `ps5-web` only build artifacts; they do not run hardware tests.
`ps5-test` builds the ELF, uploads the test bundle, and runs all 63 aggregate
tests. `ps5-suite` adds the lifetime checks. `RUN_TIMEOUT` is set to 120
seconds for the aggregate run.

The deployment root defaults to `/data/python`. Override it when needed with
`PS5_RUNTIME_ROOT=/some/other/absolute/path`.

See [docs/testing.md](docs/testing.md) for the `/data/python` versus
`/download0` storage decision and the intentional error tests.

See [docs/stdlib-status.md](docs/stdlib-status.md) for the exact standard-
library coverage, omitted APIs, PS5 limitations, upstream sources, and tests.
Update that document whenever a standard-library module changes.

See [docs/ps5-limitations.md](docs/ps5-limitations.md) for the consolidated
split between remaining implementation work and deep PS5 kernel/SDK limits.

The PS5 build automatically downloads and builds the pinned OpenSSL 3.5.2
static dependency under `build/ps5/deps/openssl`. To build it independently,
run `bash tools/build_openssl_ps5.sh` from WSL. The resulting `_ssl` and
`_hashlib` modules are linked into the interpreter.

Multiple applications can be packaged independently. See
[docs/app-bundles.md](docs/app-bundles.md) and the complete example in
[apps/flask_dashboard](apps/flask_dashboard). Validate and deploy it with:

```sh
make host-app APP=apps/flask_dashboard
PS5_HOST=192.168.4.30 make ps5-app APP=apps/flask_dashboard
```

The web deployment ships a practical starter suite under `apps/`: a Flask
runtime dashboard, storage and LAN file browsers, SQLite notes, network
checks, log and Markdown viewers, a webhook inspector, a static-site starter,
and a media catalog, plus a regular `system_report` script. Each web app
listens on its own documented port so they can run together under the process
supervisor, and every app card accepts optional command-line arguments.

Prepare an app's pure-Python dependencies on the host with
`make package-app APP=apps/myapp`; the PS5 receives the completed bundle and
does not run pip or download packages.

The browser-based manager is documented in
[docs/web-launcher.md](docs/web-launcher.md). Start it with:

```sh
PS5_HOST=192.168.4.30 make ps5-web
```

The Run script view submits bounded source bodies to the native
`POST /api/script/run` endpoint and shares persistent interpreter state with
WebREPL. Packaged applications run as independent process-backed jobs and can
be launched and stopped concurrently; file-backed scripts remain on the
web-launcher roadmap.

The PS5 configure path uses the FreeBSD compatibility triplet expected by the
SDK (`x86_64-pc-freebsd`) and the tracked patch in
`patches/ps5-freebsd-configure.patch`. Optional modules that require a
separate math library are excluded from the core target.
