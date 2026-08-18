"""PS5 adaptations of CPython socket and select event-loop coverage."""

import select
import socket
import sys


if not hasattr(select, "poll"):
    print("test_network: SKIP (select.poll unavailable on host)")
    raise SystemExit(0)


server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
connection = None
try:
    server.bind(("127.0.0.1", 0))
    server.listen(1)
    server.setblocking(False)
    poller = select.poll()
    poller.register(server, select.POLLIN)
    assert poller.poll(0) == []

    client.connect(server.getsockname())
    events = poller.poll(1000)
    assert events and events[0][1] & select.POLLIN
    connection, _ = server.accept()
    connection.setblocking(False)
    try:
        connection.recv(1)
    except BlockingIOError:
        pass
    else:
        raise AssertionError("empty non-blocking receive did not block")

    client.send(b"poll")
    poller.unregister(server)
    poller.register(connection, select.POLLIN)
    events = poller.poll(1000)
    assert events and events[0][1] & select.POLLIN
    assert connection.recv(4) == b"poll"
finally:
    if connection is not None:
        connection.close()
    client.close()
    server.close()

print("test_network: PASS")
