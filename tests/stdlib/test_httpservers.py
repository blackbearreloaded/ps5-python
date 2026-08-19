"""Threaded WSGI lifecycle check adapted from CPython server tests."""

import threading
from http.client import HTTPConnection
from socketserver import ThreadingMixIn
from wsgiref.simple_server import WSGIRequestHandler, WSGIServer, make_server


class ThreadedWSGIServer(ThreadingMixIn, WSGIServer):
    daemon_threads = True
    allow_reuse_address = True


started = threading.Event()
release = threading.Event()
errors = []


def application(environ, start_response):
    path = environ["PATH_INFO"]
    if path == "/hold":
        started.set()
        if not release.wait(5):
            raise RuntimeError("timed out waiting for threaded release")
    body = path.encode("ascii")
    start_response("200 OK", [("Content-Length", str(len(body)))])
    return [body]


server = make_server(
    "127.0.0.1", 0, application,
    server_class=ThreadedWSGIServer,
    handler_class=WSGIRequestHandler,
)
server.timeout = 5
serve_thread = threading.Thread(target=server.serve_forever)
serve_thread.start()


def request_hold():
    try:
        client = HTTPConnection(*server.server_address, timeout=5)
        client.request("GET", "/hold")
        response = client.getresponse()
        assert response.status == 200
        assert response.read() == b"/hold"
        client.close()
    except BaseException as exc:
        errors.append(exc)


hold_thread = threading.Thread(target=request_hold)
hold_thread.start()
assert started.wait(5)

client = HTTPConnection(*server.server_address, timeout=5)
client.request("GET", "/fast")
response = client.getresponse()
assert response.status == 200
assert response.read() == b"/fast"
client.close()

release.set()
hold_thread.join(5)
server.shutdown()
server.server_close()
serve_thread.join(5)

assert not hold_thread.is_alive()
assert not serve_thread.is_alive()
assert not errors, errors

print("test_httpservers: PASS")
