"""Gunicorn 23 sync master/worker loopback lifecycle.

The request and shutdown assertions are adapted from CPython 3.14.7's
``Lib/test/test_wsgiref.py`` and ``Lib/test/test_httpservers.py``.  The
process boundary follows the POSIX fork/wait checks in ``test_os.py`` and
``test_signal.py``.
"""

import os
import select
import signal
import socket
import sys
import traceback


if not hasattr(os, "fork") or not hasattr(os, "waitpid"):
    print("test_gunicorn_server: SKIP (POSIX fork boundary only)")
    raise SystemExit(0)

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "third_party"))

from gunicorn.app.base import Application


def wsgi_app(environ, start_response):
    assert environ["REQUEST_METHOD"] == "GET"
    assert environ["PATH_INFO"] == "/health"
    body = b"gunicorn-ps5-ok"
    start_response("200 OK", [("Content-Type", "text/plain"),
                               ("Content-Length", str(len(body)))])
    return [body]


def wait_for_request(address):
    request = (b"GET /health HTTP/1.1\r\n"
               b"Host: localhost\r\n"
               b"Connection: close\r\n\r\n")
    last_error = None
    for _ in range(120):
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.settimeout(0.25)
        try:
            client.connect(address)
            client.sendall(request)
            chunks = []
            while True:
                part = client.recv(4096)
                if not part:
                    break
                chunks.append(part)
            response = b"".join(chunks)
            if (b"HTTP/1.1 200 OK" not in response or
                    b"gunicorn-ps5-ok" not in response):
                raise AssertionError("unexpected Gunicorn response: %r" % response)
            return
        except (OSError, AssertionError) as exc:
            last_error = exc
            select.select([], [], [], 0.025)
        finally:
            client.close()
    raise AssertionError("Gunicorn listener did not become ready: %r" % last_error)


class TestApplication(Application):
    def __init__(self, address, inherited_fd=None):
        self.address = address
        self.inherited_fd = inherited_fd
        super().__init__(prog="gunicorn-ps5-test")

    def load_config(self):
        if self.inherited_fd is None:
            bind = "%s:%d" % self.address
        else:
            bind = "fd://%d" % self.inherited_fd
        self.cfg.set("bind", bind)
        self.cfg.set("workers", 1)
        self.cfg.set("worker_class", "sync")
        self.cfg.set("accesslog", None)
        self.cfg.set("errorlog", "-")
        self.cfg.set("loglevel", "critical")
        self.cfg.set("daemon", False)
        self.cfg.set("preload_app", False)

    def load(self):
        return wsgi_app


def run_server(use_inherited_fd):
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.bind(("127.0.0.1", 0))
    address = listener.getsockname()
    if use_inherited_fd:
        listener.listen(8)
        inherited_fd = listener.fileno()
    else:
        listener.close()
        inherited_fd = None

    pid = os.fork()
    if pid == 0:
        try:
            TestApplication(address, inherited_fd).run()
        except SystemExit as exc:
            code = exc.code if isinstance(exc.code, int) else 1
            os._exit(code)
        except BaseException:
            traceback.print_exc()
            os._exit(70)
        os._exit(0)

    if use_inherited_fd:
        listener.close()
    try:
        wait_for_request(address)
        os.kill(pid, signal.SIGTERM)
        _, status = os.waitpid(pid, 0)
        assert os.WIFEXITED(status), "Gunicorn master did not exit normally"
        assert os.WEXITSTATUS(status) == 0, "Gunicorn master exit status %d" % os.WEXITSTATUS(status)
    finally:
        try:
            os.kill(pid, signal.SIGKILL)
        except OSError:
            pass
        try:
            os.waitpid(pid, 0)
        except ChildProcessError:
            pass


run_server(False)
run_server(True)

print("test_gunicorn_server: PASS")
