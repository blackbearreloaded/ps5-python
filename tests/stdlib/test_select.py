"""PS5 adaptation of CPython Lib/test/test_selectors.py select coverage."""

import select
import socket


server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    server.bind(("127.0.0.1", 0))
    server.listen(1)
    client.connect(server.getsockname())
    connection, _ = server.accept()
    try:
        client.send(b"ready")
        readable, writable, exceptional = select.select([connection], [], [], 1.0)
        assert connection in readable
        assert not writable
        assert not exceptional
        assert connection.recv(5) == b"ready"
    finally:
        connection.close()
finally:
    client.close()
    server.close()

print("test_select: PASS")
