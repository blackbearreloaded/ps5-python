# CPythonPS5 Web Launcher Roadmap

This roadmap describes the next evolution of the PS5 Python launcher into a
small, polished Python workspace. The runtime target remains CPython **3.14.7**
from the pinned upstream commit in [`CPYTHON_VERSION.txt`](CPYTHON_VERSION.txt).

The guiding idea is a fast local developer tool: open the launcher, see the
interpreter immediately, run code or scripts, inspect the console, and know
what the PS5 is doing without needing a separate payload terminal.

## Current baseline

Already available:

- Persistent CPython 3.14.7 interpreter embedded in `python-web.elf`.
- Browser terminal-style WebREPL with history, multiline input, Ctrl+L screen
  clearing, interpreter reset, and URL-persisted views.
- Raw TCP REPL beside the HTTP server (`PS5_WEB_PORT + 1` by default), usable
  with `rlwrap nc` and sharing interpreter state with the browser.
- Application discovery, one-at-a-time app launch, live output, status, logs,
  and a separate test ELF.
- `/user/temp` as the runtime temporary directory and a documented
  self-contained application packaging workflow.
- Startup diagnostics for the HTTP and TCP endpoints.

The current launcher is intentionally trusted-LAN software. It has no
authentication, encryption, user accounts, or hostile-client isolation.

## Priority roadmap

| Priority | Workstream | Outcome |
| --- | --- | --- |
| P0 | Editor foundation | Syntax-highlighted, keyboard-friendly Python editing |
| P0 | Script runner | A dedicated editor for complete scripts, separate from the REPL |
| P0 | Process control | Track, stop, and inspect running applications |
| P1 | System dashboard | Firmware, model, memory, CPU, temperature, storage, and uptime |
| P1 | Launcher refactor | Smaller native/frontend modules with clear ownership |
| P2 | Product polish | A memorable, fast “Python Studio on PS5” experience |

## P0 — Editor foundation

### Syntax highlighting in the interpreter

Replace the plain `<textarea>` prompt with an editor surface that keeps the
current seamless terminal experience while highlighting Python tokens. The
editor must support the existing prompt flow rather than turning every command
into a separate modal dialog.

Recommended approach:

1. Vendor a small browser editor bundle at build/deploy time; nothing should
   download from the Internet on the PS5.
2. Prefer a lightweight CodeMirror-style editor or a custom overlay editor
   before considering Monaco. Monaco is powerful, but its asset size and worker
   model are likely excessive for this launcher.
3. Keep the terminal transcript read-only and make only the active prompt line
   editable.
4. Preserve the current keyboard contract: Enter evaluates, Shift+Enter adds a
   line, Up/Down recalls history, and Ctrl+L clears the visible screen.

Acceptance criteria:

- Python strings, comments, numbers, keywords, decorators, and errors are
  visibly distinct.
- Pasting a multiline block does not corrupt indentation or prompt state.
- The editor remains responsive on a long transcript and on a small viewport.
- A no-editor fallback remains available if the optional asset is unavailable.

### Interpreter auto-complete

Add completion after `.` and for common Python names. Completion should be
useful without pretending to be a full language server.

Suggested levels:

- Level 1: client-side Python keywords, builtins, imported module names, and
  history entries.
- Level 2: a small `/api/repl/complete` request that evaluates safe symbol
  inspection in the persistent interpreter and returns names/docstrings.
- Level 3: optional signature and docstring previews using `inspect`, bounded
  by time and result size.

The server must never execute arbitrary completion text as a side effect. A
completion request should inspect existing objects only, enforce limits, and
return an explicit “unavailable” result when inspection is unsafe or too
expensive.

## P0 — Dedicated script runner

Add a third top-level menu: **Run script**. This is separate from both the
Applications menu and the interactive Interpreter.

The script view should provide:

- A full-height Python editor with syntax highlighting.
- New, open, save, save-as, and run actions.
- A small file browser rooted at `/data/python/user` or another explicit user
  workspace, never an unrestricted filesystem browser.
- Run output in the existing live console surface.
- Run, stop, and rerun controls with the current exit code and elapsed time.
- A dirty-buffer indicator and confirmation before discarding unsaved edits.
- Optional “run selection” for quick experiments.

Native/API boundary:

- `POST /api/script/run` accepts a bounded script or a server-side workspace
  path and starts it through the same launcher process manager as applications.
- `POST /api/script/stop` requests cooperative termination, then escalates
  according to the process policy below.
- `GET /api/files` lists only the permitted workspace root.
- Large scripts should be uploaded in bounded chunks or saved first; avoid
  putting an unbounded source file in a single HTTP request.

Acceptance criteria:

- A user can write and run a complete script without pasting it line by line
  into the REPL.
- A script can import the same bundled standard library and packaged libraries
  as an application.
- A failed script leaves a readable traceback and does not wedge the launcher.
- Script execution and REPL evaluation retain the existing runtime lock rules.

## P0 — Application tracking and stop controls

The current launcher reports one app at a time but does not yet expose a proper
job model. Introduce a process/job manager with:

- Stable job ID, app or script name, PID, start time, current state, exit code,
  and last output cursor.
- States such as queued, starting, running, stopping, finished, failed, and
  stopped.
- A UI process card with elapsed time, recent output, and a clear Stop button.
- `POST /api/jobs/<id>/stop` with a graceful-first policy: request a normal
  termination, wait a bounded interval, then use the supported stronger signal
  only when necessary.
- A startup recovery record so the UI can explain a job that disappeared after
  a console restart.

Safety rules:

- Never enumerate or broadcast signals. Every stop request must use a PID/job
  previously issued by this launcher.
- Do not reset the shared interpreter while a Python job is running.
- Make the UI show when a stop is requested but not yet confirmed.
- Keep the existing test-only PID recovery helper separate from the normal UI
  stop path.

Acceptance criteria:

- A long-running demo can be stopped from the browser and from the TCP/HTTP
  control surface.
- The final state and exit reason are visible without refreshing the page.
- A stopped job cannot leave the launcher in a permanently “running” state.

## P1 — PS5 system dashboard

Add a compact top-bar/system drawer with capability-aware telemetry:

- PS5 firmware version.
- Console model and platform identifier.
- Launcher/runtime version and CPython version.
- Process memory and available memory, with peak usage where supported.
- CPU count, current load, and optional per-core information.
- CPU/APU temperature when exposed by the PS5 SDK or payload capability.
- Storage/free-space information for the configured `/data/python` area.
- Uptime, launcher PID, HTTP port, TCP REPL port, and active job count.
- Network address and connection health.

Implementation plan:

1. Add a native `system_info` capability adapter rather than scattering SDK
   calls through the web handler.
2. Return a versioned `/api/system` response with `value`, `unit`, `source`,
   and `available` fields.
3. Prefer official SDK/libkernel or existing payload wrappers where available;
   unsupported metrics must render as “Unavailable”, never as guessed values.
4. Poll slowly (for example, 2–5 seconds) and avoid collecting expensive
   telemetry on every WebSocket frame.
5. Keep the dashboard useful on hardware where temperature or firmware APIs
   are not exposed.

The dashboard should be treated as a capability matrix. Firmware/model and
basic memory are likely first; temperature, load, and detailed hardware data
may remain SDK- or kernel-dependent.

## P1 — Native and frontend structure

### Native launcher split

Refactor the current large `src/cpython_web_launcher.c` into focused modules:

```text
src/web/
  launcher_main.c       argument parsing, lifecycle, shutdown
  http_server.c/.h      libmicrohttpd routes and static files
  websocket.c/.h        RFC 6455 framing and live event fan-out
  tcp_repl.c/.h         raw line-oriented REPL listener
  runtime_bridge.c/.h   CPython evaluation/reset and stream capture
  job_manager.c/.h      app/script lifecycle and stop policy
  system_info.c/.h      PS5 capability adapters and telemetry
  web_state.c/.h        shared status, logs, and synchronization
```

The split should preserve behavior first. Each module gets a narrow header and
unit-testable host fallback where possible. The final ELF must keep the same
static-linking model and must not introduce a dynamic runtime dependency.

### Frontend split

Move the current single-page JavaScript into browser-sized modules:

```text
web/
  index.html
  app.css
  app.js
  state.js
  api.js
  socket.js
  terminal.js
  interpreter.js
  script-editor.js
  applications.js
  system-panel.js
  components.js
```

Use a small deploy-time bundling step if module loading becomes awkward on the
target. Keep source maps and an unbundled development mode on the host.

## P2 — Frontend ideas that make the project stand out

These are deliberately secondary to a reliable editor and process model:

- Command palette (`Ctrl+K`) for switching views, running scripts, clearing
  output, restarting the interpreter, and opening system information.
- Multiple console tabs: Interpreter, active app, script runner, and system
  events, with per-tab scrollback.
- Split view for source on the left and live output on the right.
- “Copy as script” for moving a successful REPL experiment into the script
  editor.
- Runtime badges showing CPython 3.14.7, PS5 capability level, and whether an
  operation is host-only, PS5-verified, or unavailable.
- A small package/app manifest inspector showing entry point, bundled pure-
  Python dependencies, size, and deployment timestamp.
- Log filters for stdout, stderr, launcher events, TCP REPL events, and system
  telemetry.
- Dark/light/high-contrast themes, font-size controls, and a compact mode for
  TV displays.
- A first-run welcome panel with three copyable examples: REPL expression,
  saved script, and packaged application.
- Responsive layout designed for a controller/TV as well as a desktop browser.
- A read-only “health” card that explains why a feature is unavailable instead
  of silently failing.

## Testing and release gates

Every phase should preserve the existing rules:

- Use official CPython `Lib/test` sources for standard-library behavior.
- Add host tests for protocol/state transformations and PS5 smoke tests for
  socket, process, and SDK behavior.
- Keep separate checks for HTTP/WebSocket REPL and raw TCP REPL.
- Test CR, LF, and CRLF clients, including `rlwrap nc` behavior.
- Test refresh/navigation state, long scrollback, reset, Ctrl+L, script stop,
  and app stop.
- Verify that `sys.exit()` cannot terminate the launcher process.
- Commit every completed increment and keep `Progress.md` and the relevant
  documentation status current.

## Suggested implementation order

1. Refactor the frontend state/API/socket code without changing behavior.
2. Add the editor bundle and syntax-highlighted Interpreter prompt.
3. Add bounded completion and history-aware suggestions.
4. Introduce the job manager and browser Stop control.
5. Add the Run script menu and workspace file API.
6. Split the native launcher around the stabilized job/runtime boundaries.
7. Add the capability-based system dashboard.
8. Finish command palette, tabs, themes, and TV/controller polish.

The milestone for calling this a usable “Python Studio on PS5” is not visual
polish alone: a user must be able to discover the interpreter, write or run a
script, see exactly what is running, stop it safely, and understand the
console’s hardware/runtime limits from one page.

