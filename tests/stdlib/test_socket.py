"""PS5 adaptation of CPython Lib/test/test_socket.py."""

import socket


server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    client.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    server.bind(("127.0.0.1", 0))
    server.listen(1)
    server.settimeout(2.0)
    assert server.gettimeout() == 2.0
    server.setblocking(True)
    address = server.getsockname()
    client.connect(address)
    connection, peer = server.accept()
    try:
        assert connection.getpeername()[0] == "127.0.0.1"
        assert peer[0] == "127.0.0.1"
        assert client.getsockname()[0] == "127.0.0.1"
        assert client.send(b"hello") == 5
        assert connection.recv(5) == b"hello"
        assert connection.send(b"world") == 5
        assert client.recv(5) == b"world"
        client.sendall(b"makefile")
        stream = connection.makefile("rb")
        assert stream.read(8) == b"makefile"
        stream.close()
        connection.shutdown(socket.SHUT_RDWR)
    finally:
        connection.close()
finally:
    client.close()
    server.close()

infos = socket.getaddrinfo("localhost", None, socket.AF_INET,
                           socket.SOCK_DGRAM)
assert infos
assert infos[0][0] == socket.AF_INET

receiver = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sender = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
try:
    receiver.bind(("127.0.0.1", 0))
    receiver.settimeout(2.0)
    sender.sendto(b"udp", receiver.getsockname())
    payload, address = receiver.recvfrom(16)
    assert payload == b"udp"
    assert address[0] == "127.0.0.1"
finally:
    sender.close()
    receiver.close()

if getattr(socket, "has_ipv6", False):
    ipv6 = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    try:
        ipv6.bind(("::1", 0))
    finally:
        ipv6.close()

print("test_socket: PASS")
