<h1 align="center">Python-PS5</h1>

<p align="center">
  <strong>CPython for the PlayStation 5</strong><br>
  A practical Python runtime, browser launcher, and app supervisor for PS5 homebrew.
</p>

<p align="center">
  <a href="https://github.com/blackbearreloaded/ps5-python/actions/workflows/ci.yml?query=branch%3Amain"><img src="https://github.com/blackbearreloaded/ps5-python/actions/workflows/ci.yml/badge.svg?branch=main" alt="CI status"></a>
  <a href="https://github.com/blackbearreloaded/ps5-python/releases/latest"><img src="https://img.shields.io/github/v/release/blackbearreloaded/ps5-python?display_name=tag&amp;sort=semver&amp;label=latest%20release" alt="Latest release"></a>
  <a href="https://github.com/blackbearreloaded/ps5-python/releases"><img src="https://img.shields.io/github/downloads/blackbearreloaded/ps5-python/total?label=downloads" alt="GitHub downloads"></a>
  <a href="https://github.com/blackbearreloaded/ps5-python/blob/main/LICENSE"><img src="https://img.shields.io/github/license/blackbearreloaded/ps5-python" alt="License"></a>
  <img src="https://img.shields.io/badge/CPython-3.14.7-3776AB?logo=python&amp;logoColor=white" alt="CPython 3.14.7">
  <img src="https://img.shields.io/badge/platform-PS5-003791?logo=playstation&amp;logoColor=white" alt="PlayStation 5">
</p>

Experimental but functional port of the CPython language runtime to the
jailbroken PS5 payload environment.

This repository currently targets **CPython 3.14.7**, pinned to upstream source
commit `823f0323ee6ec1402088b73bce1a38473cac36dc`. The exact pin is recorded in
[CPYTHON_VERSION.txt](CPYTHON_VERSION.txt); this is not an unpinned “latest
Python” port.

The project now ships a standalone interpreter, a browser-based Python manager,
process-backed application jobs, and a tested standard-library subset. It is
still intended for development and homebrew payload environments, not stock
retail consoles.

## What is included

- CPython **3.14.7**, pinned to upstream commit
  `823f0323ee6ec1402088b73bce1a38473cac36dc`.
- Standalone and web-launcher ELF artifacts built with the PS5 Payload SDK.
- 170 of 189 pinned top-level standard-library entries present in the runtime
  bundle (about 90% module presence; not API-parity coverage).
- Browser interpreter, script editor, app arguments, and independently
  stoppable process-backed applications.
- Practical examples for Flask, SQLite, storage, networking, files, logs,
  Markdown, static sites, webhooks, and regular scripts.

The remaining limitations are explicit: ordinary `subprocess` execution,
process pools, IPv6, GUI/PTY stacks, in-console packaging/bootstrap, and some
native extension paths are outside the current PS5 payload contract.

## Documentation

| Document | Purpose |
| --- | --- |
| [Project status](docs/status.md) | Concise implementation, verification, and limitation matrix |
| [Standard-library status](docs/stdlib-status.md) | Coverage inventory, tested APIs, omissions, and upstream-derived checks |
| [PS5 limitations](docs/ps5-limitations.md) | Platform, kernel, SDK, GUI, PTY, subprocess, and packaging boundaries |
| [Web launcher guide](docs/web-launcher.md) | Browser manager, API, app lifecycle, arguments, and deployment |
| [App bundles](docs/app-bundles.md) | Self-contained app manifests, dependencies, assets, and modes |
| [Testing guide](docs/testing.md) | Host checks, PS5 aggregate tests, lifetime tests, and recovery |
| [Release guide](docs/releasing.md) | Tags, release assets, self-hosted PS5 builds, and local fallback |
| [Contributing](CONTRIBUTING.md) | Development workflow and pull-request checklist |
| [Third-party notices](THIRD_PARTY_NOTICES.md) | Upstream projects, build-time dependencies, and attribution |

The implementation roadmap is in [roadmap.md](roadmap.md). Test provenance and portable-subset
rules are documented in [tests/UPSTREAM_TESTS.md](tests/UPSTREAM_TESTS.md).

## Project references

| Project | Role |
| --- | --- |
| [CPython](https://github.com/python/cpython/tree/v3.14.7) | Pinned interpreter and standard-library source |
| [PS5 Payload SDK](https://github.com/ps5-payload-dev/sdk) | Prospero compiler, linker, headers, and ELF deployment tooling |
| [PacBrew packages](https://github.com/ps5-payload-dev/pacbrew-repo) | PS5 payload ecosystem and packaged build dependencies |
| [Flask](https://github.com/pallets/flask) | Practical web-app example |
| [Gunicorn](https://github.com/benoitc/gunicorn) | Constrained synchronous WSGI serving example |
| [OpenSSL](https://github.com/openssl/openssl) | Static TLS and cryptography dependency |
| [SQLite](https://www.sqlite.org/) | Static database dependency and example app backend |
| [zlib](https://github.com/madler/zlib), [bzip2](https://sourceware.org/bzip2/), [XZ Utils](https://github.com/tukaani-project/xz) | Compression dependencies |
| [libffi](https://github.com/libffi/libffi), [libmicrohttpd](https://git.gnunet.org/libmicrohttpd.git/) | Native runtime and web-launcher dependencies |

The PS5 SDK is a build-time prerequisite and is not copied into this repository.
See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) before redistributing
builds or bundled dependencies.

## Status

The implementation snapshot is summarized in a table in
[`docs/status.md`](docs/status.md). In brief, the project builds CPython 3.14.7
for PS5, passes the 68-script aggregate suite, includes 170 of 189 pinned
stdlib entries, and provides a browser launcher with independently stoppable
process-backed applications. Module-level coverage is in
[`docs/stdlib-status.md`](docs/stdlib-status.md); platform boundaries are in
[`docs/ps5-limitations.md`](docs/ps5-limitations.md).

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
PS5_HOST="<your-PS5-IP>" make ps5-run
PS5_HOST="<your-PS5-IP>" make ps5-run SCRIPT=examples/main.py
PS5_HOST="<your-PS5-IP>" make ps5-run SCRIPT=tests/core_suite.py
```

The target is not a native app package yet; it is intentionally one ELF, one
external Python script, and one small runtime directory.

Phase 4 lifetime checks are available with:

```sh
make host-lifetime
PS5_HOST="<your-PS5-IP>" make ps5-lifetime
```

Run the aggregate hardware tests, including the standard-library checks, with:

```sh
PS5_HOST="<your-PS5-IP>" make ps5-test
```

Run those tests plus the repeated-process lifetime checks with:

```sh
PS5_HOST="<your-PS5-IP>" make ps5-suite
```

The live TLS smoke test can also be run directly when external DNS and internet
access are available:

```sh
PS5_HOST="<your-PS5-IP>" make ps5-run SCRIPT=tests/stdlib/test_tls_handshake.py
```

`ps5-core` and `ps5-web` only build artifacts; they do not run hardware tests.
`ps5-test` builds the ELF, uploads the test bundle, and runs all 68 aggregate
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
PS5_HOST="<your-PS5-IP>" make ps5-app APP=apps/flask_dashboard
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
PS5_HOST="<your-PS5-IP>" make ps5-web
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

## CI and releases

The [CI workflow](.github/workflows/ci.yml) runs the host suite and shell
validation on every push and pull request. PS5 hardware tests remain an
explicit operator step because the SDK and a jailbroken console are not
available on GitHub-hosted runners.

The [release workflow](.github/workflows/release.yml) builds the standalone
and web ELF artifacts on a self-hosted runner labeled `ps5-sdk`, clones and
installs the PS5 Payload SDK into temporary runner storage, packages the
runtime bundle, and uploads the ELF files, archive, and checksums to an
existing GitHub Release. [docs/releasing.md](docs/releasing.md) also provides
the local `gh release upload` fallback.

## License

Python-PS5 is licensed under the [GNU General Public License v3.0](LICENSE).
Bundled third-party components retain their respective licenses; see
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for attribution and details.
