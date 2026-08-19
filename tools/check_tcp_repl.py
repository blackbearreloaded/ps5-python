"""Check the line-oriented TCP REPL beside the web launcher."""

import argparse
import socket


def read_until(connection, marker):
    data = b""
    while marker not in data:
        chunk = connection.recv(1024)
        if not chunk:
            raise RuntimeError("TCP REPL closed before its prompt")
        data += chunk
    return data


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", required=True)
    parser.add_argument("--port", type=int, required=True)
    args = parser.parse_args()
    with socket.create_connection((args.host, args.port), timeout=5) as connection:
        connection.settimeout(5)
        banner = read_until(connection, b">>> ")
        connection.sendall(b"print(123)\r")
        output = read_until(connection, b">>> ")
        connection.sendall(b"1 + 1\r\n")
        output += read_until(connection, b">>> ")
    if b"123" not in output or b"2" not in output or b">>> " not in banner:
        raise AssertionError(f"unexpected TCP REPL output: {banner + output!r}")
    print("TCP_REPL_CHECK: PASS")


if __name__ == "__main__":
    main()
