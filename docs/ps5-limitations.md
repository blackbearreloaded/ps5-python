# CPython 3.14.7 PS5 limitations and remaining work

This document separates ordinary implementation work from constraints imposed
by the current PS5 payload, kernel, and SDK. Most of the requested pure-Python
standard library is already bundled; the remaining gaps are documented below.

## Potentially possible with project work

These items are feasible with additional Python, C, build, packaging, or PS5
SDK work. They do not require changing the CPython language runtime itself.

- Complete upstream regression coverage for modules that already work.
- Finish advanced `dataclasses`, `typing`, `pathlib`, `datetime`, `json`, `re`,
  `struct`, compression, XML, and profiler edge cases.
- Bundle a CA certificate store for verified HTTPS.
- Bundle timezone data for `zoneinfo`.
- Add pure-Python or statically linked DBM backends.
- Implement `venv --without-pip` for isolated application directories.
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
- Port missing legacy pure-Python compatibility modules such as `smtpd`.

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
- **Desktop GUI stack:** `tkinter`, `turtle`, and GUI-dependent functionality
  require Tcl/Tk, which the PS5 target does not provide.
- **Curses and desktop browser integration:** there is no conventional
  terminal desktop or browser process to control.
- **Windows-only modules:** `msvcrt`, `winreg`, and `winsound` do not apply to
  PS5.
- **Full GNU readline/editline support:** no linkable backend is currently
  shipped.
- **True standard shared-memory/process-pool semantics:** require the
  unavailable mmap, semaphore, and process-launch primitives.

A kernel-assisted ELF/process broker could remove some process-related
blockers, but that would be a separate native PS5 subsystem, not a normal
CPython-library implementation.

## Packaging implication

The intended deployment model is self-contained: pip runs on the development
machine or CI, universal pure-Python dependencies are placed in the app bundle,
and the completed bundle is deployed to PS5. The console does not need pip,
internet access, a compiler, or source-distribution build tools.

See [`app-bundles.md`](app-bundles.md) for the preparation workflow and
[`stdlib-status.md`](stdlib-status.md) for module-by-module status and tests.
