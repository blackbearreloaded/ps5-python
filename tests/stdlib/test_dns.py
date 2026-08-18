"""PS5 DNS resolution check for an external ASCII hostname."""

import socket


infos = socket.getaddrinfo("google.com", 80, socket.AF_INET, socket.SOCK_STREAM)
addresses = []
for family, kind, protocol, _, address in infos:
    if family == socket.AF_INET and kind == socket.SOCK_STREAM:
        if address[0] not in addresses:
            addresses.append(address[0])

assert addresses
print("test_dns: google.com -> " + ", ".join(addresses))
print("test_dns: PASS")
