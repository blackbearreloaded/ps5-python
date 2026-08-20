# CPython 3.14.7 PS5 limitations and remaining work

This document separates ordinary implementation work from constraints imposed
by the current PS5 payload, kernel, and SDK. Most of the requested pure-Python
standard library is already bundled; the remaining gaps are documented below.

## Inventory boundary

The pinned CPython 3.14.7 `Lib/` comparison has 190 raw top-level source
entries, including the non-stdlib `site-packages` directory. The current bundle
inventory contains 170 of the 189 actual stdlib entries; 19 are absent. That is a roughly
90% module-presence estimate, not a claim of 90% API parity. Full API parity
and complete upstream regression coverage are lower and are tracked separately
in [`stdlib-status.md`](stdlib-status.md).

This is a static source/bundle inventory. The final deployed bundle passes the
68-script PS5 aggregate suite; the raw absence count is 20 only because it
also includes `site-packages`.

The current 19-entry absent set is fully classified as 17 blocked entries
(2 packaging/bootstrap, 4 GUI, 2 PTY/terminal, 5 platform-specific, and 4
demos), and 2 deliberately unnecessary
private/fallback files.

## Potentially possible with project work

These items are feasible with additional Python, C, build, packaging, or PS5
SDK work. They do not require changing the CPython language runtime itself.

- Complete upstream regression coverage for modules that already work.
- Expand the focused coverage for the newly bundled portable modules and the
  `_strptime`/`_markupbase` dependency closure into upstream-derived tests.
- Finish advanced `dataclasses`, `typing`, `pathlib`, `datetime`, `json`, `re`,
  `struct`, compression, XML, and profiler edge cases.
- Bundle a CA certificate store for verified HTTPS.
- Bundle timezone data for `zoneinfo`.
- Add pure-Python or statically linked DBM backends.
- Implement `venv --without-pip` and selected `ensurepip` support if an
  in-console environment workflow becomes a requirement.
- Improve `site-packages`, `.pth`, `importlib.metadata`, and package metadata
  handling for self-contained app bundles.
- Extend the host-side package preparation workflow around
  [`tools/package_app.py`](../tools/package_app.py).
- Implement `time.sleep()` through a PS5-native wait primitive if the SDK
  exposes one.
- Add Pipe-based or custom IPC alternatives for `multiprocessing.Queue` and
  `Semaphore`.
- Add a custom process-pool abstraction using the supported explicit `fork()`
  path.
- Improve `readline`, `pdb`, `code`, and terminal behavior with a PS5-specific
  console layer.
- Expand HTTP/WSGI stress coverage and HTTPS server support.
- Add more Gunicorn functionality that does not require unsupported process
  execution.
- Add static PS5-native bridges for selected `ctypes` use cases.
- Add static MarkupSafe/native acceleration if performance requires it.

Some of these items may require native C or SDK work rather than only Python
changes.

## Blocked by deep PS5 kernel or SDK limitations

These cannot be made fully compatible by copying additional CPython files. A
native platform feature or a separate kernel-assisted broker would be needed.

- **General subprocess execution:** `execve()` cannot launch ordinary
  filesystem PS5 ELFs. This blocks normal `subprocess`, `spawn`, `forkserver`,
  `ProcessPoolExecutor`, Gunicorn reloader/daemon paths, and Flask
  debugger/reloader subprocesses.
- **File-descriptor duplication:** `dup()` and `dup2()` return `ENOTSUP`,
  affecting subprocess setup, descriptor inheritance, and some POSIX
  hardening.
- **POSIX named semaphores:** unavailable, preventing standard
  `multiprocessing.Queue` and `Semaphore` behavior.
- **File-backed `mmap`:** returns `ENOTSUP`, preventing standard
  `multiprocessing.shared_memory.SharedMemory` behavior and several
  shared-state designs.
- **IPv6:** disabled because the PS5 SDK's `getaddrinfo()` behavior fails
  CPython's IPv6 configuration requirements.
- **Arbitrary dynamic extension loading:** general `.sprx`/`.so` loading via
  `ctypes` is not available as a normal CPython capability.
- **Desktop GUI stack:** `idlelib`, `tkinter`, `turtle`, and `turtledemo`
  require Tcl/Tk or another desktop GUI stack, which the PS5 target does not
  provide.
- **PTY/terminal integration:** `curses` and `pty` require a conventional
  interactive terminal or pseudo-terminal environment outside the PS5 target.
- **Host/platform-only entries:** the `_aix_support`, `_android_support`,
  `_apple_support`, `_ios_support`, and `_osx_support` entries describe
  non-PS5 platform behavior and are not target requirements.
- **Demo entries:** `__hello__`, `__phello__`, `antigravity`, and `this` are
  intentionally omitted because they provide no PS5 runtime capability.
- **Windows-only modules:** `msvcrt`, `winreg`, and `winsound` do not apply to
  PS5.
- **Full GNU readline/editline support:** no linkable backend is currently
  shipped.
- **True standard shared-memory/process-pool semantics:** require the
  unavailable mmap, semaphore, and process-launch primitives.

A kernel-assisted ELF/process broker could remove some process-related
blockers, but that would be a separate native PS5 subsystem, not a normal
CPython-library implementation.

The portable helper closure includes private/deprecated files because the goal
is to close the selected upstream source inventory, not to imply that each file
is a public API requirement. Separately, `_pydatetime` and `_pydecimal` are
deliberately unnecessary because native `_datetime` and `_decimal` provide
the corresponding runtime paths. `_strptime` and `_markupbase` are in the
current bundle because they have known public-module dependency impact. No
absent entry should be treated as a promise of full upstream parity. The
aggregate run records the PS5-specific `netrc` representation of an omitted
account as an empty string rather than desktop CPython's `None`.

## Packaging implication

The intended deployment model is self-contained: pip runs on the development
machine or CI, universal pure-Python dependencies are placed in the app bundle,
and the completed bundle is deployed to PS5. The console does not need pip,
internet access, a compiler, or source-distribution build tools.

See [`app-bundles.md`](app-bundles.md) for the preparation workflow and
[`stdlib-status.md`](stdlib-status.md) for module-by-module status and tests.
