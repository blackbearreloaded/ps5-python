# Python web launcher

The project includes a native PS5 ELF that serves a browser-based manager for
the Python app bundles.

```text
python-web.elf
 ├── libmicrohttpd HTTP server
 ├── WebSocket live-output bridge
 ├── app manifest scanner
 ├── CPython app runner
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

Port 8080 is intentionally avoided because the existing PS5 `websrv` commonly
uses it. Override the port with `PS5_WEB_PORT`.

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

The deployment command detaches the payload loader, so the web manager remains
running after the build command exits. Launch it from Homebrew Launcher or a
payload manager using the same ELF if desired.

## API

| Endpoint | Purpose |
| --- | --- |
| `/` | Browser manager page |
| `/api/apps` | Lists app IDs and display names |
| `/api/status` | Reports launch state and exit code |
| `/api/launch?app=hello` | Starts one app bundle |
| `/api/logs?since=0` | Returns new stdout/stderr bytes and `X-Log-Next` |
| `/api/logs/clear` | Clears the server-side log buffer and connected consoles |
| `/api/shutdown` | Stops the manager; useful for tests |
| `/ws` | Streams JSON log and status events over WebSocket |

The page makes one `/ws` attempt per page load and prefers it for live output.
If the upgrade is interrupted, it falls back to the cursor API at one-second
intervals without repeatedly reconnecting. Refreshing the page tries WebSocket
again. The cursor API remains available for diagnostics and automation.

WebSocket messages are JSON objects. Log events have the form
`{"type":"log","data":"..."}`; status events contain `type`, `running`,
`finished`, and `exit_code` fields. A newly connected browser receives the
current status and buffered output before live events.

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
the manager. A direct WebSocket handshake can be verified separately against
`ws://<PS5-IP>:8090/ws`.
