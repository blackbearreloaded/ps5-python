# WebREPL reference comparison

This note records the implementation patterns reviewed before extending the
PS5 WebREPL. The launcher targets CPython **3.14.7** and is not a
MicroPython-compatible device, but the transport and session rules are useful
references.

## MicroPython `webrepl`

The official project is primarily a browser client and protocol reference. It
uses one WebSocket connection and multiplexes terminal traffic with optional
file-transfer/control messages. Terminal input and output are WebSocket text
messages; file operations use binary messages. The design intentionally exposes
one shared REPL session rather than a separate interpreter per browser
connection, and it is intended to run in the background while the application
continues running.

Reference: <https://github.com/micropython/webrepl>

The browser client converts pasted LF characters to CR before sending terminal
input and writes incoming text directly to a terminal widget:

<https://raw.githubusercontent.com/micropython/webrepl/master/webrepl.js>

## `socketserverREPL`

`iwanders/socketserverREPL` is a separate pure-Python TCP design. It creates a
`code.InteractiveConsole` for each connection, reads complete lines with
`readline()`, and routes stdout/stderr to that connection using
`threading.local()`. Its `ThreadingMixIn` server enables concurrent clients,
sets `SO_REUSEADDR`, and shuts down through an explicit server loop.

Reference: <https://github.com/iwanders/socketserverREPL>

This is useful for the line-buffering and stream-routing ideas, but its
per-connection scope is deliberately different from the single-session
MicroPython model.

## `pyWebREPL`

`xg590/pyWebREPL` is a host-side automation client for ESP32/ESP8266. It
reverse-engineers and emits MicroPython WebREPL WebSocket frames, including
paste mode (`Ctrl-E`) for sending a complete code block. It does not provide a
replacement server implementation.

Reference: <https://github.com/xg590/pyWebREPL>

## PS5 implementation mapping

The current launcher already keeps the HTTP/WebSocket daemon and a persistent
CPython 3.14.7 interpreter inside the same `python-web.elf` process. It also
serializes app and REPL execution, captures stdout/stderr per evaluation, and
terminates source input before calling the CPython evaluator. The automated
WebREPL check covers both `print(123)` and interactive expression display (`1 +
1` → `2`).

The main intentional difference is the wire envelope: the PS5 manager sends
JSON status/log/REPL events so the browser can share one socket with the app
manager. It is therefore not a drop-in client for the raw MicroPython terminal
protocol. File-transfer frames are not implemented yet.

For shell clients, the launcher also provides a separate raw TCP REPL listener.
It accepts newline-terminated source lines and emits plain output plus `>>>`
prompts, so tools such as `rlwrap nc` can be used without WebSocket support.
It runs on `PS5_WEB_PORT + 1` by default (or `PS5_REPL_PORT` when configured),
because the HTTP/WebSocket and raw TCP protocols cannot share one listening
socket.

## Recommended compatibility boundary

Keep the existing JSON manager socket stable for the launcher UI. If a raw
MicroPython-compatible terminal is needed later, add a separate WebSocket path
with one active session, raw text terminal frames, CR/LF normalization, and
`code.InteractiveConsole`-style incomplete-block buffering. Do not replace the
current socket with per-connection interpreter instances: that would make
browser tabs observe different globals and would undermine the persistent
session model.
