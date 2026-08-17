import socket


HOST = "0.0.0.0"
PORT = 9091

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind((HOST, PORT))
server.listen(4)
print("Socket server listening on {0}:{1}".format(HOST, PORT), flush=True)

while True:
    connection, address = server.accept()
    print("Connection from {0}:{1}".format(address[0], address[1]), flush=True)
    try:
        while True:
            data = connection.recv(4096)
            if not data:
                break
            print("NETCAT: " + data.decode("utf-8", "replace").rstrip("\r\n"), flush=True)
            connection.send(b"OK\n")
    finally:
        connection.close()
        print("Connection closed", flush=True)
