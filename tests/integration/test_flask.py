"""Flask/Werkzeug compatibility smoke test for CPython 3.14.7 on PS5.

The request, WSGI, cookie, and escaping assertions are adapted from the
pinned CPython ``Lib/test/test_wsgiref.py``, ``test_http_cookies.py``, and
``test_http_cookiejar.py`` behavior checks.  The vendored dependency closure
is Flask 3.1.3, Werkzeug 3.1.8, Jinja2 3.1.6, MarkupSafe 3.0.3,
ItsDangerous 2.2.0, Click 8.2.1, and Blinker 1.9.0.
"""

from pathlib import Path
import os
import select
import signal
import socket
import sys


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "third_party"))

from flask import Flask, render_template_string, request, session
from gunicorn.app.base import Application
from werkzeug.test import Client
from werkzeug.wrappers import Response


app = Flask("ps5_flask")
app.secret_key = "ps5-flask-test-secret"


@app.get("/health")
def health():
    return {"status": "ok", "method": request.method}


@app.post("/echo")
def echo():
    return {"value": request.get_json()["value"]}


@app.get("/template")
def template():
    return render_template_string("<p>{{ value }}</p>", value="<unsafe>")


@app.get("/set-session")
def set_session():
    session["user"] = "ps5"
    return "set"


@app.get("/get-session")
def get_session():
    return session.get("user", "missing")


with app.test_client() as client:
    response = client.get("/health")
    assert response.status_code == 200
    assert response.get_json() == {"status": "ok", "method": "GET"}

    response = client.post("/echo", json={"value": "from-flask"})
    assert response.status_code == 200
    assert response.get_json() == {"value": "from-flask"}

    response = client.get("/template")
    assert response.status_code == 200
    assert response.data == b"<p>&lt;unsafe&gt;</p>"

    assert client.get("/set-session").status_code == 200
    assert client.get("/get-session").data == b"ps5"

# Flask's client is a Werkzeug client; instantiate the underlying WSGI client
# directly as a second boundary check.
client = Client(app, Response)
assert client.get("/health").status_code == 200


class FlaskGunicornApplication(Application):
    def __init__(self, address):
        self.address = address
        super().__init__(prog="flask-gunicorn-ps5-test")

    def load_config(self):
        self.cfg.set("bind", "%s:%d" % self.address)
        self.cfg.set("workers", 1)
        self.cfg.set("worker_class", "sync")
        self.cfg.set("accesslog", None)
        self.cfg.set("errorlog", "-")
        self.cfg.set("loglevel", "critical")
        self.cfg.set("daemon", False)

    def load(self):
        return app


if hasattr(os, "fork") and hasattr(os, "waitpid"):
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.bind(("127.0.0.1", 0))
    address = listener.getsockname()
    listener.close()
    pid = os.fork()
    if pid == 0:
        try:
            FlaskGunicornApplication(address).run()
        except SystemExit as exc:
            os._exit(exc.code if isinstance(exc.code, int) else 1)
        except BaseException:
            os._exit(70)

    try:
        request_bytes = (b"GET /health HTTP/1.1\r\nHost: localhost\r\n"
                         b"Connection: close\r\n\r\n")
        response = b""
        for _ in range(120):
            connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            connection.settimeout(0.25)
            try:
                connection.connect(address)
                connection.sendall(request_bytes)
                while True:
                    chunk = connection.recv(4096)
                    if not chunk:
                        break
                    response += chunk
                break
            except OSError:
                select.select([], [], [], 0.025)
            finally:
                connection.close()
        assert b"200 OK" in response
        assert b'"status":"ok"' in response or b'"status": "ok"' in response
        os.kill(pid, signal.SIGTERM)
        _, status = os.waitpid(pid, 0)
        assert os.WIFEXITED(status) and os.WEXITSTATUS(status) == 0
    finally:
        try:
            os.kill(pid, signal.SIGKILL)
        except OSError:
            pass
        try:
            os.waitpid(pid, 0)
        except ChildProcessError:
            pass

print("test_flask: PASS")
