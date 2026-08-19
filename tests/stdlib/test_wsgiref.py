"""Loopback WSGI checks adapted from CPython 3.14.7 ``test_wsgiref``.

The upstream test module exercises the full unittest/support harness and
several signal and CGI paths.  This portable subset keeps the important
server boundary: a WSGI application receives the request environment and a
real HTTP client receives the generated response over a loopback socket.
"""

import threading

from http.client import HTTPConnection
from wsgiref.simple_server import make_server
from wsgiref.util import setup_testing_defaults
from wsgiref.validate import validator


def application(environ, start_response):
    setup_testing_defaults(environ)
    body = (
        environ["REQUEST_METHOD"] + " " +
        environ["PATH_INFO"] + "?" + environ["QUERY_STRING"] + " " +
        environ["HTTP_X_TEST"]
    ).encode("ascii")
    start_response("200 OK", [
        ("Content-Type", "text/plain"),
        ("Content-Length", str(len(body))),
    ])
    return [body]


server = make_server("127.0.0.1", 0, validator(application))
server.timeout = 5
thread = threading.Thread(target=server.handle_request)
thread.start()
try:
    host, port = server.server_address
    client = HTTPConnection(host, port, timeout=5)
    client.request("GET", "/wsgi?target=ps5", headers={"X-Test": "loopback"})
    response = client.getresponse()
    body = response.read()
    client.close()
finally:
    thread.join(5)
    server.server_close()

assert not thread.is_alive()
assert response.status == 200
assert response.getheader("Content-Type") == "text/plain"
assert body == b"GET /wsgi?target=ps5 loopback"

print("test_wsgiref: PASS")
