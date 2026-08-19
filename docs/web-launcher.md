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
```

The browser UI is kept outside the ELF in the tracked web/ directory:
web/index.html, web/app.css, and web/app.js.
Deployment uploads these files to /data/python/web/. The native server only
serves the static files and owns the API/runtime boundary, so the UI can be
restyled or extended without rebuilding the Python runtime.

The default URL is:

```text
http://<PS5-IP>:8090/
```

The Interpreter menu is the first and default view. Its URL is
`?view=interpreter`; the Applications menu uses `?view=applications`.

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
| `/api/status` | Reports the launcher PID, launch state, exit code, and TCP REPL port |
| `/api/launch?app=hello` | Starts one app bundle |
| `/api/logs?since=0` | Returns new stdout/stderr bytes and `X-Log-Next` |
| `/api/logs/clear` | Clears the server-side log buffer and connected consoles |
| `/api/repl/reset` | Restarts the embedded interpreter and clears its globals |
| `/api/shutdown` | Stops the manager; useful for tests |
| `/ws` | Streams JSON log/status events and accepts WebREPL source lines |

The page makes one `/ws` attempt per page load and prefers it for live output.
If the upgrade is interrupted, it falls back to the cursor API at one-second
intervals without repeatedly reconnecting. Refreshing the page tries WebSocket
again. The cursor API remains available for diagnostics and automation.

WebSocket messages are JSON objects. Log events have the form
`{"type":"log","data":"..."}`; status events contain `type`, `running`,
`pid`, `finished`, and `exit_code` fields. A newly connected browser receives the
current status and buffered output before live events.

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
line, Shift+Enter inserts a newline for a block, and the up/down arrows recall
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

The TCP REPL sends a CPython prompt, accepts one source line per newline, and
returns raw stdout/stderr or expression display text followed by the next
`>>>` prompt. It is intended for trusted LAN use and does not provide
authentication or encryption.

`sys.exit()` is contained by the embedded runtime: the client receives
`SystemExit` and a fresh prompt, while the launcher ELF and other sessions keep
running. A bare `exit` is not automatically installed in this isolated build;
it behaves like any other unresolved Python name unless the user imports or
defines it.

The protocol comparison and compatibility boundary for the reference
implementations are recorded in
[`docs/webrepl-reference.md`](webrepl-reference.md). That note distinguishes
the official MicroPython terminal protocol from the separate
`socketserverREPL` server pattern and the `pyWebREPL` host-side client.

For test recovery when a payload is wedged, read `pid` from `/api/status` and
use the test-only helper documented in
[`docs/ps5-process-recovery.md`](ps5-process-recovery.md). The helper requires
an already-known PID and does not enumerate or broadcast signals.

The manager runs one app at a time. After an app exits, the same page can launch
it again or select another app; the output buffer is reset for each run and
after the final exit message, so finished-app logs are not replayed to a new
portal visit. The Clear button also clears the server buffer and all connected
browser consoles. While an app is running, including a long-lived daemon, its
live output remains available.

## Automated validation

The deployment script can run an end-to-end HTTP check:

```sh
PS5_HOST=192.168.4.30 PS5_WEB_CHECK=1 make ps5-web
```

This lists the apps, starts `hello`, fetches its live output, and shuts down
the manager. It also evaluates `print(123)` and `1 + 1` through both the
embedded WebREPL and the raw TCP REPL. Input is line-terminated before
evaluation, and expression results are captured explicitly so clients behave
like an interactive prompt. A direct WebSocket handshake can be verified
separately against
`ws://<PS5-IP>:8090/ws` with `tools/check_web_repl.py`.
