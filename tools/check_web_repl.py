"""Check the embedded WebREPL over the launcher WebSocket."""

import argparse
import base64
import hashlib
import json
import os
import socket
import struct


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


def read_exact(connection, size):
    data = b""
    while len(data) < size:
        chunk = connection.recv(size - len(data))
        if not chunk:
            raise RuntimeError("WebSocket closed before a complete frame")
        data += chunk
    return data


def read_frame(connection):
    first, second = read_exact(connection, 2)
    length = second & 0x7F
    if length == 126:
        length = struct.unpack(">H", read_exact(connection, 2))[0]
    elif length == 127:
        length = struct.unpack(">Q", read_exact(connection, 8))[0]
    payload = read_exact(connection, length)
    return first & 0x0F, payload


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
        if b"101 Switching Protocols" not in headers or expected.encode() not in headers:
            raise RuntimeError("WebSocket upgrade failed")
        # The server sends status first, followed by the current log buffer.
        read_frame(connection)
        connection.sendall(frame("1 + 2"))
        while True:
            opcode, payload = read_frame(connection)
            if opcode == 1:
                event = json.loads(payload.decode())
                if event.get("type") == "repl":
                    assert event.get("ok") is True
                    assert "3" in event.get("data", "")
                    break
        connection.sendall(frame(b"", opcode=8))
    print("WEBREPL_CHECK: PASS")


if __name__ == "__main__":
    main()
