"""CPython 3.14.7-derived checks for the portable wsgiref surface."""

import http.client
import threading

from wsgiref.headers import Headers
from wsgiref.simple_server import make_server
from wsgiref.util import setup_testing_defaults


defaults = {}
setup_testing_defaults(defaults)
assert defaults["REQUEST_METHOD"] == "GET"
assert defaults["wsgi.version"] == (1, 0)

headers = Headers([("Content-Type", "text/plain")])
assert headers["content-type"] == "text/plain"

seen = {}


def application(environ, start_response):
    seen.update(environ)
    start_response("200 OK", [("Content-Type", "text/plain")])
    return [
        (environ["PATH_INFO"] + "?" + environ["QUERY_STRING"]).encode("ascii")
    ]


server = make_server("127.0.0.1", 0, application)
thread = threading.Thread(target=server.handle_request)
thread.start()
try:
    connection = http.client.HTTPConnection(*server.server_address, timeout=5)
    connection.request("GET", "/hello?x=1")
    response = connection.getresponse()
    body = response.read()
    connection.close()
finally:
    thread.join(timeout=5)
    server.server_close()

assert not thread.is_alive()
assert response.status == 200
assert body == b"/hello?x=1"
assert seen["REQUEST_METHOD"] == "GET"
assert seen["QUERY_STRING"] == "x=1"

print("test_wsgiref: PASS")
