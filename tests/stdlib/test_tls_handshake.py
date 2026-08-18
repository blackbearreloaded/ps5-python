"""Optional live TLS handshake check for the PS5 socket/OpenSSL boundary."""

import os
import socket
import ssl
import sys


# Keep the host suite deterministic; this test is for a PS5 deployment.
if not sys.platform.startswith("freebsd"):
    print("test_tls_handshake: SKIP (PS5 runtime only)")
else:
    host = os.getenv("CPYTHONPS5_TLS_HOST", "www.google.com")
    port = int(os.getenv("CPYTHONPS5_TLS_PORT", "443"))
    context = ssl.create_default_context()
    # Phase 1 proves the native socket/OpenSSL handshake. Certificate-store
    # loading is a separate follow-up once a PS5 CA bundle is selected.
    context.check_hostname = False
    context.verify_mode = ssl.CERT_NONE

    with socket.create_connection((host, port), timeout=10.0) as raw:
        with context.wrap_socket(raw, server_hostname=host) as tls:
            assert tls.version() is not None
            tls.sendall(
                ("HEAD / HTTP/1.0\r\nHost: " + host +
                 "\r\nConnection: close\r\n\r\n").encode("ascii")
            )
            assert tls.recv(1)

    print("test_tls_handshake: PASS")
