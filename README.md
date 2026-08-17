# CPythonPS5

Experimental port of the CPython language runtime to the jailbroken PS5 payload environment.

The first target is deliberately small: execute Python language semantics without depending on the Python standard library or third-party modules. The runtime will eventually be embedded in a native PS5 application, but this repository begins with a standalone interpreter proof of concept.

## Scope of the first milestone

- Cross-compile a released CPython version with the PS5 SDK.
- Start the standalone CPython ELF directly.
- Execute a script supplied as a separate file, for example `main.py`.
- Validate expressions, functions, classes, exceptions, generators, comprehensions, and garbage collection.

The first milestone does not include the standard-library package tree, networking, SQLite, `pip`, native extension loading, GUI support, or subprocess APIs. The interpreter still contains CPython’s required built-in runtime modules; the test script itself must remain import-free. Initial hardware testing uses one ELF plus an external `/data/python/main.py` file.

## References

- [CPython](https://github.com/python/cpython)
- [MicroPython](https://github.com/micropython/micropython)
- [PS5 PacBrew packages](https://github.com/ps5-payload-dev/pacbrew-repo)

See [PLAN.md](PLAN.md) for the implementation roadmap and acceptance criteria.
The language-core test provenance and portable-subset rules are documented in
[`tests/UPSTREAM_TESTS.md`](tests/UPSTREAM_TESTS.md).

## Status

Core scaffold started. The pinned CPython checkout is kept outside Git under
`upstream/cpython`; run `make source-fetch` to recreate it. The host-side
language-core suite and its PS5 aggregate run pass. The PS5 core ELF and
static library build with WSL and the installed PS5 SDK.

## Build the first PS5 ELF

Run these from WSL at the repository root:

```sh
make source-fetch
make host-build
make ps5-core
```

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

`ps5-core` and `ps5-web` only build artifacts; they do not run hardware tests.
`ps5-test` builds the ELF, uploads the test bundle, and runs all 24 aggregate
tests. `ps5-suite` adds the lifetime checks. `RUN_TIMEOUT` is set to 120
seconds for the aggregate run.

The deployment root defaults to `/data/python`. Override it when needed with
`PS5_RUNTIME_ROOT=/some/other/absolute/path`.

See [docs/testing.md](docs/testing.md) for the `/data/python` versus
`/download0` storage decision and the intentional error tests.

See [docs/stdlib-status.md](docs/stdlib-status.md) for the exact standard-
library coverage, omitted APIs, PS5 limitations, upstream sources, and tests.
Update that document whenever a standard-library module changes.

Multiple applications can be packaged independently. See
[docs/app-bundles.md](docs/app-bundles.md) and the complete example in
[apps/hello](apps/hello). Validate and deploy it with:

```sh
make host-app APP=apps/hello
PS5_HOST=192.168.4.30 make ps5-app APP=apps/hello
```

The browser-based manager is documented in
[docs/web-launcher.md](docs/web-launcher.md). Start it with:

```sh
PS5_HOST=192.168.4.30 make ps5-web
```

The PS5 configure path uses the FreeBSD compatibility triplet expected by the
SDK (`x86_64-pc-freebsd`) and the tracked patch in
`patches/ps5-freebsd-configure.patch`. Optional modules that require a
separate math library are excluded from the core target.
