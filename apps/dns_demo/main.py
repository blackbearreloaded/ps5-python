import os
import socket


def resolve(hostname):
    addresses = []
    for family, kind, protocol, _, address in socket.getaddrinfo(
        hostname, 80, socket.AF_INET, socket.SOCK_STREAM
    ):
        if family == socket.AF_INET and kind == socket.SOCK_STREAM:
            if address[0] not in addresses:
                addresses.append(address[0])
    return addresses


localhost = resolve("localhost")
assert localhost
print("DNS localhost: " + ", ".join(localhost), flush=True)

external = os.environ.get("CPYTHONPS5_DNS_HOST", "")
if external:
    try:
        print("DNS {0}: {1}".format(external, ", ".join(resolve(external))), flush=True)
    except OSError as error:
        print("DNS {0}: unavailable ({1})".format(external, error), flush=True)
else:
    print("DNS external: skipped (set CPYTHONPS5_DNS_HOST to probe)", flush=True)

print("DNS demo: PASS", flush=True)
