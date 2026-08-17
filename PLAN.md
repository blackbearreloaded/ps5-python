# CPythonPS5 implementation plan

## 1. Project goal

Build the smallest useful CPython runtime that can execute Python language code as a PS5 payload.

This is not initially a normal desktop Python installation. It is a runtime experiment with a narrow PS5 process boundary. The first success criterion is:

```text
PS5 ELF -> initialize CPython -> open external file.py -> execute it -> return a verified result
```

The first test deliberately uses one standalone interpreter ELF and one external
import-free `file.py`. It does not require a native PS5 app package, a custom
launcher, an installed `PATH`, or an embedded script. A native embedding API is
kept as a later option after the standalone interpreter is proven on hardware.

## 2. Reference projects

### CPython

Use [CPython](https://github.com/python/cpython) as the language implementation and compatibility target.

Study these areas first:

- `Python/` — interpreter runtime and initialization.
- `Objects/` — built-in object implementations.
- `Parser/` and `Grammar/` — parsing and compilation.
- `Python/ceval.c` and related code — bytecode evaluation.
- `Programs/` — executable entry points and build helpers.
- `Modules/Setup` — static and built-in module selection.
- `Include/` — public and internal runtime interfaces.
- `Tools/build/` — generated files and frozen-module tooling.

Start from a released CPython tag. Keep PS5-specific build files and compatibility patches separate from upstream source.

### MicroPython

Use [MicroPython](https://github.com/micropython/micropython) as a reference for constrained targets, not as the primary implementation.

Study its port-specific layout, board startup boundary, heap allocation, frozen code, linker integration, and small platform hooks. MicroPython can guide the PS5 host design, but replacing CPython with MicroPython would not meet this project's compatibility goal.

## 3. Explicit first-milestone non-goals

Do not implement these until the language-core tests pass:

- broad standard-library coverage (the first `os`/`time` slice is now included);
- user `import` and module search paths;
- `os`, `pathlib`, `socket`, `subprocess`, or environment inspection;
- `sqlite3`, SSL, HTTP, or compression;
- `ctypes` and dynamic extension modules;
- `pip`, wheels, package installation, or virtual environments;
- `multiprocessing` and fork/exec behavior;
- curses, Tk, SDL, Dear ImGui, or any GUI;
- a REPL or terminal emulator;
- Python application packaging.

## 4. Phase 0 — repository and build audit

### Tasks

1. Identify the exact PS5 SDK release and supported compiler, linker, libc, threading, and startup conventions.
2. Record the existing ELF payload build commands used by the workspace.
3. Locate a host-native CPython build that can generate CPython build artifacts.
4. Select a released CPython version rather than tracking `main`.
5. Record the source checksum and preserve upstream notices.
6. Produce a standalone interpreter ELF plus a static `libpython` artifact for later embedding work.

### Deliverables

- `docs/porting-notes.md` describing the PS5 toolchain.
- A reproducible host build command.
- A pinned CPython version and source checksum.
- A list of required, available, and unavailable platform APIs.

### Exit criteria

The toolchain builds both a trivial PS5 ELF and the selected CPython source on the host.

## 5. Phase 1 — minimal CPython build

### Objective

Compile CPython with no optional libraries and no external Python installation.

### Tasks

1. Run a normal host build first to understand generated files.
2. Add a PS5 cross-compilation configuration instead of broadly rewriting upstream code.
3. Disable tests, `ensurepip`, optional extension modules, shared-module loading, and desktop-specific features.
4. Build required bootstrap pieces as static or frozen components.
5. Keep the user module search path limited to the explicit external runtime bundle.
6. Provide the minimal ASCII/UTF-8 bootstrap codecs needed by CPython startup.
7. Keep the normal GIL/threaded build; do not attempt free-threaded CPython yet.

### Deliverables

- Separate `build/host` and `build/ps5` outputs.
- A minimal static runtime artifact.
- A standalone `python.elf` launcher and generated `cpython-lib` bundle.
- A generated configuration record showing which modules are present.

### Exit criteria

The PS5 linker produces an ELF containing CPython without unresolved symbols from optional modules.

## 6. Phase 2 — standalone PS5 interpreter test

### Objective

Run the CPython interpreter ELF directly through the existing PS5 payload
workflow and pass it a script path as its first argument.

### Initial file contract

The first hardware bundle is intentionally simple:

```text
/data/python/python.elf
/data/python/main.py
/data/python/cpython-lib/
```

The interpreter receives the script path as its first argument and an optional
runtime directory as its second argument. The generated bundle contains only
the codec files required to initialize CPython; the user script remains
import-free. Do not add a native app package or PS5-specific UI just to test
language execution. The deployment root is configurable through
`PS5_RUNTIME_ROOT`, but `/data/python` is the default.

### First script

```python
x = 10
y = 32
result = x + y
```

The script should verify that `result == 42` and report a short success/failure
message. The first hardware run should also include one deliberate syntax error
and one uncaught exception to confirm failure reporting.

### Exit criteria

The PS5 starts the standalone ELF, opens the external script, executes it, and
returns a successful result without a user `import` or a native app package.

The launcher is intentionally still a standalone ELF rather than a native app
package. Native app packaging and a richer embedding API remain later phases.

### App bundle prototype

The first multi-app layout is now implemented:

```text
/data/python/runtime/python.elf
/data/python/runtime/cpython-lib/
/data/python/apps/<id>/app.json
/data/python/apps/<id>/main.py
/data/python/apps/<id>/lib/
/data/python/apps/<id>/assets/
```

The launcher accepts optional app-root and app-library paths, sets `__file__`,
and adds both app paths to CPython's module search path. A native controller
launcher can later select a manifest and invoke this same app contract.

### Web launcher prototype

The native `python-web.elf` manager now serves a browser UI, lists manifests,
launches one selected app, and exposes both a cursor-based API and a WebSocket
live stdout/stderr stream. It uses libmicrohttpd for HTTP and HTTP upgrade
handling, with a small built-in RFC 6455 frame bridge. The static dependency
is built under `build/ps5/deps/` and linked into the ELF. It uses the shared
runtime under `/data/python/runtime/` and listens on port 8090 by default.

## 7. Phase 3 — language-core validation

Create a small collection of external scripts. Each script returns or prints a
simple result so the process wrapper can verify it without a Python test
framework.

### Required test groups

1. Numeric operations and comparisons.
2. Strings and Unicode literals.
3. Lists, tuples, dictionaries, and sets.
4. Functions, default arguments, closures, and recursion.
5. Classes, inheritance, methods, and attribute lookup.
6. Exceptions and `try`/`finally`.
7. Comprehensions and generator expressions.
8. Iterators and `for` loops.
9. Allocation, reference counting, cyclic garbage collection, and repeated execution.

### Test design

- No `unittest`.
- No `pytest`.
- No user imports or standard-library filesystem dependency.
- One shell/PowerShell test runner with clear pass/fail output.
- A host-side equivalent runner for fast iteration.
- Derive behavior from the pinned CPython `Lib/test` suite; document each
  adapted test in `tests/UPSTREAM_TESTS.md`.
- Keep the PS5 subset import-free; the complete upstream regression suite is a
  later host-only gate until its standard-library dependencies are available.

### Exit criteria

All selected language-core tests pass on both host and PS5, with no crash after repeated execution cycles where supported.

## 8. Phase 4 — memory and lifetime validation

1. Report the native process peak resident-size value when the PS5 exposes it
   through `getrusage(RUSAGE_SELF)`.
2. Exercise repeated allocation and release in a bounded script.
3. Run multiple fresh interpreter processes to validate initialization and
   finalization across process lifetimes.
4. Test syntax errors, uncaught exceptions, recursion limits, and
   allocation-failure handling where practical.
5. Confirm that the runtime stays inside the intended process memory. The
   first measurement is an OS-reported peak, not a CPython allocator profile.
6. Keep the native shutdown path explicit and record whether finalization is
   reliable in the PS5 process model.

The implemented Phase 4 checks are in `tests/lifetime/`, with host execution
through `make host-lifetime` and repeated PS5 execution through
`PS5_HOST=192.168.4.30 make ps5-lifetime`. The intentional syntax and uncaught
exception scripts are host-automated and can be deployed individually on the
PS5 to verify the native debug path.

### Exit criteria

Stress scripts complete without crashes, and failures reach the native debug path.

## 9. Phase 5 — output and interaction boundary

Only after the core works, add a tiny host interface:

- `print()` or an equivalent routed to PS5 debug output;
- script evaluation from a memory buffer;
- optional command/result exchange over the existing HTTP debugging channel;
- no general-purpose standard library yet.

The first interface should remain deterministic and one-way. A full REPL is not required.

## 10. Phase 6 — optional standard-library expansion

This phase is outside the first target. Add modules one at a time, each with a PS5 test:

1. `_io` and file-backed script loading.
2. A safe `os` subset mapped to PS5 paths.
3. `json`.
4. `sqlite3` using the PacBrew SQLite package.
5. `socket` and HTTP.
6. `ssl`.
7. Compression modules.
8. Dynamic extension loading, only if the loader and security model support it cleanly.

Every module must be optional at build time. The core interpreter must continue to build without it.

## 11. Proposed repository layout after implementation starts

```text
CPythonPS5/
├── README.md
├── PLAN.md
├── LICENSES/
├── docs/
│   ├── porting-notes.md
│   ├── runtime-boundary.md
│   └── testing.md
├── patches/
│   └── cpython-ps5-*.patch
├── platform/
│   ├── ps5_config.h
│   ├── ps5_memory.c
│   ├── ps5_threads.c
│   └── ps5_host.c
├── host/
│   └── run-core-tests.*
├── tests/
│   ├── core_basic.py
│   ├── core_objects.py
│   ├── core_exceptions.py
│   └── core_gc.py
└── build/
    └── ... generated and ignored ...
```

Do not create this full layout until the corresponding files are needed.

## 12. Technical risks

### Startup assumptions

CPython startup may assume more POSIX behavior than the first no-import script suggests. Track missing functions from linker errors and runtime failures instead of implementing a broad compatibility layer in advance.

### Memory model

The PS5 payload environment may differ from desktop Unix in virtual memory, thread stacks, signals, and process lifetime. Keep the first build single-purpose and measure actual hardware behavior.

### Output

The interpreter may initialize successfully while `print()` has nowhere useful to go. Native result extraction and PS5 debug reporting must be tested independently from Python stdout.

### Source size

CPython is much larger than MicroPython. Use MicroPython's port structure and build discipline, but do not assume MicroPython's footprint or platform assumptions transfer to CPython.

### Upstream drift

Pin one CPython release. Do not begin by tracking CPython `main`; update deliberately after a working port exists.

## 13. Definition of done for version 0.1

Version 0.1 is complete when:

- a clean checkout builds the PS5 ELF with documented commands;
- the standalone ELF starts through the normal PS5 payload workflow;
- an external Python script executes without user imports;
- arithmetic, objects, functions, classes, exceptions, generators, and garbage collection pass;
- results and errors reach the native debug path;
- memory usage and limitations are documented;
- no standard-library or networking dependency is required;
- the initial Git history identifies the upstream CPython version used.

## 14. Immediate next action

Use the passing core suite as the baseline for Phase 4 memory and lifetime
validation. Add more adapted cases from the pinned `Lib/test` tree only when
they remain import-free or the required standard-library dependency is added
explicitly.
