# Project status

This is the current implementation snapshot for the Python-PS5 port. Module-
level coverage and individual API limitations are tracked separately in
[`stdlib-status.md`](stdlib-status.md).

| Area | Status | Verified or included | Main limitations |
| --- | --- | --- | --- |
| CPython runtime | Implemented | CPython 3.14.7, standalone interpreter ELF, web launcher ELF, and static runtime library | Requires the PS5 Payload SDK and a payload-capable console; not for stock retail firmware |
| Build and tests | Verified | Host checks, 68-script PS5 aggregate suite, and repeated-process lifetime checks | Full upstream regression coverage is not yet complete |
| Standard library | Broad supported subset | 170 of 189 pinned top-level entries present in the runtime bundle; detailed inventory in [`stdlib-status.md`](stdlib-status.md) | Presence is not API-parity; GUI, PTY, bootstrap, platform-specific, and demo modules remain outside the target |
| Web launcher | Implemented | Browser interpreter, WebREPL, TCP REPL, script editor, live output, app listing, arguments, and scrolling UI | File-backed script persistence is still future work |
| Application jobs | Implemented | Packaged apps run in independent child processes with separate PIDs and targeted Stop control | This supervisor does not make the general Python `subprocess` API available |
| Practical examples | Implemented | Flask dashboard, SQLite notes, storage and LAN file browsers, network toolbox, log and Markdown viewers, webhook inspector, static site, media catalog, and regular scripts | Each app remains constrained by the runtime and PS5 filesystem/network permissions |
| Threads and processes | Partial, verified | `_thread`, `threading`, thread pools, `multiprocessing.Pipe`, and explicit-fork `Process` coverage | Queue/Semaphore, POSIX `SharedMemory`, `ProcessPoolExecutor`, and general child-ELF launching are unavailable |
| Networking and TLS | IPv4 verified | TCP/UDP sockets, selectors, asyncio readiness, DNS, HTTP, and live TLS handshakes | IPv6 and certificate verification/CA-store selection are not implemented |
| Data and compression | Implemented subset | SQLite, zlib, bzip2, xz/liblzma, ZIP, TAR, XML, and related wrappers are statically integrated and smoke-tested | Full upstream compression and archive stress coverage remains pending |
| Flask web stack | Verified subset | Vendored Flask 3.1.3, Gunicorn 23.0.0, Werkzeug, Jinja2, MarkupSafe, ItsDangerous, Click, and Blinker | Daemon/re-exec, Unix sockets, plugin entry points, reloaders, debuggers, and optional async workers are outside the target |
| Native/platform boundaries | Documented | PS5-specific filesystem, signals, memory, timing, process, and launcher behavior are tracked in [`ps5-limitations.md`](ps5-limitations.md) | `mmap`, arbitrary native-library loading, advanced descriptor duplication, GUI/Tk, curses, PTY, and subprocess execution remain limited |
| CI and releases | Implemented | GitHub CI, self-hosted PS5 release workflow, pinned CPython/SDK acquisition, ELF packaging, checksums, and release uploads | Hardware CI still requires an operator-provided PS5 and self-hosted SDK runner |

## Current focus

The remaining work is mostly platform-boundary completion and deeper validation:

- certificate-verified HTTPS with a selected PS5 CA bundle;
- a kernel-assisted ELF broker for general subprocess-compatible launching;
- broader upstream regression and long-running stress coverage;
- complete file-backed script persistence and richer deployment tooling.
