"""PS5 adaptation of CPython Lib/test/test_selectors.py."""

import selectors
import socket

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    server.bind(("127.0.0.1", 0))
    server.listen(1)
    client.connect(server.getsockname())
    connection, _ = server.accept()
    try:
        with selectors.DefaultSelector() as selector:
            selector.register(connection, selectors.EVENT_READ)
            client.sendall(b"selector")
            events = selector.select(1.0)
            assert len(events) == 1
            assert events[0][1] & selectors.EVENT_READ
            assert connection.recv(8) == b"selector"
            selector.unregister(connection)
    finally:
        connection.close()
finally:
    client.close()
    server.close()

print("test_selectors: PASS")
