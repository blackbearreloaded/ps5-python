"""Check the embedded WebREPL over the launcher WebSocket."""

import argparse
import base64
import hashlib
import json
import os
import socket
import struct
from urllib.request import urlopen


def frame(payload, opcode=1):
    payload = payload if isinstance(payload, bytes) else payload.encode()
    mask = os.urandom(4)
    masked = bytes(value ^ mask[index % 4] for index, value in enumerate(payload))
    length = len(payload)
    if length < 126:
        header = bytes((0x80 | opcode, 0x80 | length))
    elif length < 65536:
        header = bytes((0x80 | opcode, 0x80 | 126)) + struct.pack(">H", length)
    else:
        raise ValueError("test payload too large")
    return header + mask + masked


def read_frame(connection, pending=b""):
    while len(pending) < 2:
        pending += connection.recv(1024)
    first, second = pending[:2]
    pending = pending[2:]
    length = second & 0x7F
    if length == 126:
        while len(pending) < 2:
            pending += connection.recv(1024)
        length = struct.unpack(">H", pending[:2])[0]
        pending = pending[2:]
    elif length == 127:
        while len(pending) < 8:
            pending += connection.recv(1024)
        length = struct.unpack(">Q", pending[:8])[0]
        pending = pending[8:]
    while len(pending) < length:
        pending += connection.recv(1024)
    payload = pending[:length]
    return first & 0x0F, payload, pending[length:]


def evaluate(connection, pending, source, expected, ok=True, forbidden=()):
    connection.sendall(frame(source if source.endswith("\n") else source + "\n"))
    while True:
        opcode, payload, pending = read_frame(connection, pending)
        if opcode != 1:
            continue
        event = json.loads(payload.decode())
        if event.get("type") != "repl":
            continue
        if event.get("ok") is not ok:
            raise AssertionError(f"unexpected REPL status: {event!r}")
        if any(marker in event.get("data", "") for marker in forbidden):
            raise AssertionError(f"unexpected REPL data: {event!r}")
        if expected not in event.get("data", ""):
            raise AssertionError(f"unexpected REPL data: {event!r}")
        return pending


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", required=True)
    parser.add_argument("--port", type=int, required=True)
    args = parser.parse_args()
    key = base64.b64encode(os.urandom(16)).decode()
    expected = base64.b64encode(
        hashlib.sha1((key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").encode()).digest()
    ).decode()
    with socket.create_connection((args.host, args.port), timeout=5) as connection:
        connection.sendall(
            (
                "GET /ws HTTP/1.1\r\n"
                f"Host: {args.host}:{args.port}\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                f"Sec-WebSocket-Key: {key}\r\n"
                "Sec-WebSocket-Version: 13\r\n\r\n"
            ).encode()
        )
        headers = b""
        while b"\r\n\r\n" not in headers:
            headers += connection.recv(1024)
        header_end = headers.index(b"\r\n\r\n") + 4
        response_headers = headers[:header_end]
        pending = headers[header_end:]
        if b"101 Switching Protocols" not in response_headers or expected.encode() not in response_headers:
            raise RuntimeError("WebSocket upgrade failed")
        # The server sends status first, followed by the current log buffer.
        _, _, pending = read_frame(connection, pending)
        pending = evaluate(connection, pending, "print(123)", "123")
        pending = evaluate(connection, pending, "1 + 1", "2")
        for _ in range(3):
            pending = evaluate(
                connection, pending, "", "", forbidden=("SyntaxError",)
            )
        pending = evaluate(connection, pending, "import sys; sys.exit()",
                           "SystemExit", ok=False)
        pending = evaluate(connection, pending, "webrepl_reset_marker = 42", "")
        with urlopen(f"http://{args.host}:{args.port}/api/repl/reset", timeout=5) as response:
            reset = json.load(response)
        if reset.get("reset") is not True:
            raise AssertionError(f"interpreter reset failed: {reset!r}")
        pending = evaluate(
            connection, pending,
            'globals().get("webrepl_reset_marker", "missing")',
            "'missing'",
        )
        connection.sendall(frame(b"", opcode=8))
    print("WEBREPL_CHECK: PASS")


if __name__ == "__main__":
    main()
