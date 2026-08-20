# Python web launcher

The project includes a native PS5 ELF that serves a browser-based manager for
the Python app bundles.

```text
python-web.elf
 ├── libmicrohttpd HTTP server
 ├── WebSocket live-output bridge
 ├── app manifest scanner
 ├── CPython app runner
 ├── persistent CPython WebREPL session
 └── stdout/stderr log buffer

python-app-supervisor.elf
 ├── local TCP control socket
 ├── one session worker per application
 ├── one forked CPython child per application
 └── targeted stop and waitpid lifecycle
```

The browser UI is kept outside the ELF in the tracked web/ directory:
web/index.html, web/app.css, web/app.js, and the vendored Highlight.js asset
under web/vendor/highlight.js/.
Deployment uploads these files to /data/python/web/. The native server only
serves the static files, including the Highlight.js asset, and owns the
API/runtime boundary, so the UI can be restyled or extended without rebuilding
the Python runtime.

The script editor and active WebREPL input use the locally vendored
Highlight.js Python grammar. Highlighting is rendered in a read-only layer over
the native textareas, preserving paste, selection, keyboard shortcuts, and
mobile editing without requiring a browser extension or internet access.

The launcher supports three built-in visual themes: **Studio**, **Terminal**,
and **Paper**. The theme control in the top bar cycles between them and stores
the selection in browser-local storage. Themes are entirely frontend-owned, so
they do not change the PS5 runtime or require a server setting.

The default URL is:

```text
http://<PS5-IP>:8090/
```

The Interpreter menu is the first and default view. Its URL is
`?view=interpreter`; the Applications menu uses `?view=applications`; the
script workspace uses `?view=script`.

Port 8080 is intentionally avoided because the existing PS5 `websrv` commonly
uses it. Override the port with `PS5_WEB_PORT`.

The launcher also exposes a raw, line-oriented TCP REPL on the next port by
default (`PS5_WEB_PORT + 1`). For example, with the test launcher on port
9603, connect from a local shell with:

```sh
rlwrap nc 192.168.4.30 9604
```

Set `PS5_REPL_PORT` to choose another TCP port. The TCP and WebSocket REPLs
share the same persistent CPython **3.14.7** interpreter state. The HTTP port
and TCP REPL port must be different because one socket cannot simultaneously
serve HTTP/WebSocket framing and raw line-oriented traffic.

The WebSocket endpoint is `/ws`. The launcher uses libmicrohttpd for HTTP and
HTTP upgrade handling, then implements the small RFC 6455 framing layer needed
for the browser console. The static libmicrohttpd dependency is built by
`tools/build_libmicrohttpd.sh` into `build/ps5/deps/user/homebrew/` and linked
into `python-web.elf`; it is not a runtime file that must be deployed.

## Build and deploy

```sh
PS5_HOST=192.168.4.30 make ps5-web
```

The launcher is uploaded to:

```text
/data/python/runtime/python-web.elf
```

The deployment also uploads and starts `python-app-supervisor.elf`. It listens
only on `127.0.0.1` at `PS5_WEB_PORT + 2` by default; override that port with
`PS5_APP_SUPERVISOR_PORT` when needed. Packaged applications run through this
supervisor and receive their own native PID.

The build also emits `build/ps5/python-web-test.elf`, a separate test-only
copy of the launcher. Use `tools/run_ps5_web_test.sh` to deploy that artifact;
it defaults to port `9601`, leaving the production-named ELF and its port
selection independent.

The deployment command detaches the payload loader, so the web manager remains
running after the build command exits. Launch it from Homebrew Launcher or a
payload manager using the same ELF if desired.

On startup, the launcher writes separate debug lines for the running process,
the HTTP endpoint, and the TCP REPL endpoint. They are available in the live
application log and through `/api/logs`.

## API

| Endpoint | Purpose |
| --- | --- |
| `/` | Browser manager page |
| `/api/apps` | Lists app IDs and display names |
| `/api/status` | Reports launcher data plus the complete app-job list |
| `/api/launch?app=hello` | Starts an app bundle and returns its job ID/PID |
| `POST /api/app/stop?job_id=1` | Stops one selected application child |
| `POST /api/script/run` | Runs a bounded script body in the persistent interpreter |
| `/api/logs?since=0` | Returns new stdout/stderr bytes and `X-Log-Next` |
| `/api/logs/clear` | Clears the server-side log buffer and connected consoles |
| `/api/repl/reset` | Restarts the embedded interpreter and clears its globals |
| `/api/shutdown` | Stops the manager; useful for tests |
| `/ws` | Streams JSON log/status events and accepts WebREPL source lines |

The page makes one `/ws` attempt per page load and uses it as the live transport
for output and status. It does not poll `/api/logs` or `/api/status`; if the
upgrade fails, the UI reports that the live link is unavailable. Refreshing the
page tries WebSocket again. The cursor API remains available for diagnostics and
automation, but is not used as a browser refresh loop.

WebSocket messages are JSON objects. Log events have the form
`{"type":"log","data":"..."}`; status events contain the legacy selected-job
fields plus a `jobs` array. Each job includes `job_id`, `app`, `app_pid`,
`running`, `finished`, `exit_code`, and `state`. A newly connected browser
receives the current job list and buffered output before live events.

## Interactive interpreter

The **Interpreter** menu uses the same WebSocket connection as live logs. The
browser sends a masked WebSocket text frame containing Python source; the
running `python-web.elf` evaluates it in a persistent CPython **3.14.7**
interpreter and returns:

```json
{"type":"repl","ok":true,"data":"3\n"}
```

The interpreter is presented as a CLI-style terminal: commands are entered at
the prompt in the same pane where output appears. Enter evaluates the current
line, including an empty line which simply advances to a fresh prompt.
Shift+Enter inserts a newline for a block, and the up/down arrows recall
history. Ctrl+L clears the visible terminal screen without restarting the
interpreter. Expressions use interactive-display behavior, while multi-line
input is executed as a block. Variables and imports persist between
evaluations until **Restart interpreter** is selected. Opening the Interpreter
menu adds `?view=interpreter` to the URL, while Applications uses
`?view=applications`, so a browser refresh returns to the selected view.
Command text and its result are joined with a single terminal line break;
the launcher does not insert an extra blank line after each command. Long
sessions scroll inside the interpreter pane instead of expanding the page.
Output and tracebacks are captured per evaluation, so they do not corrupt the
app log stream. The runtime serializes app execution and REPL evaluation
through the interpreter lock; an app and a REPL command are never executing
Python objects concurrently.

This follows the MicroPython WebREPL pattern: the network server remains in
the ELF, the interpreter session remains local to that process, and the
browser is only a transport and terminal UI. The current implementation is
intended for trusted LAN use; it has no authentication or encryption.

The TCP REPL sends a CPython prompt, accepts one source line per newline,
including empty lines, and returns raw stdout/stderr or expression display text
followed by the next `>>>` prompt. It is intended for trusted LAN use and does
not provide authentication or encryption.

`exit()`, `quit()`, and `sys.exit()` restart the embedded interpreter and
return a fresh prompt. The launcher ELF and its control plane keep running, but
all persistent `__main__` globals are cleared. The WebSocket manager broadcasts
the reset to connected browser sessions, while the HTTP script route reports
`restarted: true`.

The protocol comparison and compatibility boundary for the reference
implementations are recorded in
[`docs/webrepl-reference.md`](webrepl-reference.md). That note distinguishes
the official MicroPython terminal protocol from the separate
`socketserverREPL` server pattern and the `pyWebREPL` host-side client.

For test recovery when a payload is wedged, read `pid` from `/api/status` and
use the test-only helper documented in
[`docs/ps5-process-recovery.md`](ps5-process-recovery.md). The helper requires
an already-known PID and does not enumerate or broadcast signals.

The manager keeps a small in-memory table of application jobs. The separate
app supervisor creates an independent session worker and forked child for each
launch, forwards output, and reports each child PID and exit state. Applications
can run concurrently, including long-lived daemons. The Active jobs panel
selects a job and sends a targeted Stop request; the supervisor sends `SIGTERM`,
waits three seconds, and then uses `SIGKILL` if necessary. The Clear button
clears the shared output buffer and connected browser consoles. The table is
intentionally not persisted because it describes live OS processes; completed
slots are reused after their reader exits.

## Script workspace

The **Run script** menu provides a complete-script editor separate from the
interactive REPL. Paste or write a Python program in the editor and select
**Run script**, or press Ctrl+Enter. The result appears in the adjacent output
pane, and unsaved edits are marked in the editor header.

The editor sends a `text/plain` `POST` body to `/api/script/run`. The launcher
accepts at most 65,536 source bytes, rejects embedded NUL bytes, evaluates the
source under the same runtime mutex as the WebREPL, and returns JSON in this
form:

```json
{"ok":true,"restarted":false,"data":"hello\\n","source_bytes":15}
```

Python exceptions return HTTP 200 with `ok: false` and the captured traceback
in `data`; malformed or oversized requests return an HTTP error. The script
shares the persistent `__main__` globals with the browser and raw TCP REPL, so
values created by one surface are visible to the others. Execution is
synchronous and bounded by the HTTP request; it is not yet an isolated job and
there is no Stop action.

The editor still does not save files, expose a workspace browser, or provide
file-backed New/Open/Save actions. Those controls, plus job-level elapsed time,
exit state, and cooperative stopping, remain part of the dedicated script
runner roadmap.

Long-lived apps run in isolated child processes managed by
`python-app-supervisor.elf`. The supervisor forwards output, reports each child
PID and exit status, and stops only the PID it created. The service does not yet
provide readiness probes or automatic restart policy. The interpreter remains
in `python-web.elf` and can be used while packaged app jobs are active.

## Automated validation

The deployment script can run an end-to-end HTTP check:

```sh
PS5_HOST=192.168.4.30 PS5_WEB_CHECK=1 make ps5-web
```

This lists the apps, starts `hello`, fetches its live output, and shuts down
the manager. It also evaluates `print(123)` and `1 + 1` through both the
embedded WebREPL and the raw TCP REPL, then posts a complete script through
`/api/script/run` and verifies its shared interpreter state. Input is
line-terminated before evaluation, and expression results are captured
explicitly so clients behave
like an interactive prompt. A direct WebSocket handshake can be verified
separately against
`ws://<PS5-IP>:8090/ws` with `tools/check_web_repl.py`.
