import os
import platform
import sys

from flask import Flask, jsonify, render_template_string

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".shared"))
from serve import serve


app = Flask("flask_dashboard")
PORT = 9101

PAGE = """<!doctype html>
<title>Flask PS5 Dashboard</title>
<style>
body{background:#101516;color:#edf0e3;font:16px system-ui;margin:40px;max-width:900px}
h1{color:#c7f36b}section{border:1px solid #344342;padding:18px;margin:14px 0;background:#182021}
dt{color:#879492;margin-top:8px}dd{margin:2px 0;font-family:monospace;word-break:break-all}
a{color:#c7f36b}
</style>
<h1>Flask PS5 Dashboard</h1>
<p>Useful baseline service for the CPython PS5 runtime.</p>
<section><h2>Runtime</h2><dl>
{% for key, value in info.items() %}<dt>{{ key }}</dt><dd>{{ value }}</dd>{% endfor %}
</dl></section>
<section><h2>Health</h2><p><a href="/api/health">GET /api/health</a></p></section>
"""


def runtime_info():
    return {
        "python": sys.version.split()[0],
        "platform": platform.platform(),
        "machine": platform.machine(),
        "hostname": platform.node() or "unknown",
        "working directory": os.getcwd(),
        "entry point": __file__,
    }


@app.get("/")
def index():
    return render_template_string(PAGE, info=runtime_info())


@app.get("/api/health")
def health():
    return jsonify({"ok": True, "service": "flask_dashboard", "python": sys.version.split()[0]})


if __name__ == "__main__":
    serve(app, PORT, "Flask dashboard")
