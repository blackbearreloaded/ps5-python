import os
import sys
import time

from flask import Flask, jsonify, render_template_string, request

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".shared"))
from serve import serve


app = Flask("log_viewer")
PORT = 9106
ROOT = os.path.abspath(os.environ.get("CPYTHONPS5_LOG_ROOT", "/data/python"))
EXTENSIONS = (".log", ".txt")

PAGE = """<!doctype html>
<title>Log Viewer</title>
<style>
body{background:#101516;color:#edf0e3;font:15px system-ui;margin:30px;max-width:1200px}
h1{color:#c7f36b}.layout{display:grid;grid-template-columns:280px 1fr;gap:18px}
aside,main{background:#182021;border:1px solid #344342;padding:16px}ul{padding-left:18px}
a{color:#c7f36b}pre{background:#0c1213;padding:14px;overflow:auto;max-height:70vh;white-space:pre-wrap}
input{background:#0c1213;color:#edf0e3;border:1px solid #344342;padding:8px;width:70%}button{padding:8px;background:#c7f36b;border:0}
.muted{color:#879492}
</style>
<h1>Log Viewer</h1><p class="muted">Root: {{ root }} · Showing the last 200 lines of a selected .log or .txt file.</p>
<div class="layout"><aside><h2>Files</h2><ul>{% for item in files %}<li><a href="/?file={{ item.path|urlencode }}">{{ item.path }}</a></li>{% else %}<li>No log files found.</li>{% endfor %}</ul></aside>
<main>{% if selected %}<h2>{{ selected }}</h2><form><input name="file" value="{{ selected }}"><input name="filter" value="{{ filter_text }}" placeholder="optional filter"><button>Apply</button></form><pre>{{ content }}</pre>{% else %}<p>Select a log file to inspect it.</p>{% endif %}</main></div>
"""


def safe_path(relative):
    candidate = os.path.realpath(os.path.join(ROOT, relative or ""))
    if candidate != ROOT and not candidate.startswith(ROOT + os.sep):
        raise ValueError("path is outside the log root")
    return candidate


def log_files():
    result = []
    for current, directories, names in os.walk(ROOT):
        depth = os.path.relpath(current, ROOT).count(os.sep)
        directories[:] = [name for name in directories if not name.startswith(".")]
        if depth >= 2:
            directories[:] = []
        for name in names:
            if name.lower().endswith(EXTENSIONS):
                path = os.path.join(current, name)
                result.append({"path": os.path.relpath(path, ROOT).replace("\\", "/")})
    return sorted(result, key=lambda item: item["path"].lower())[:100]


def read_tail(path, filter_text):
    with open(path, "r", encoding="utf-8", errors="replace") as stream:
        lines = stream.read(1024 * 1024).splitlines()[-200:]
    if filter_text:
        needle = filter_text.lower()
        lines = [line for line in lines if needle in line.lower()]
    return "\n".join(lines)


@app.get("/")
def index():
    selected = request.args.get("file", "").strip()
    filter_text = request.args.get("filter", "").strip()
    content = ""
    if selected:
        try:
            path = safe_path(selected)
            if not os.path.isfile(path) or not selected.lower().endswith(EXTENSIONS):
                return "log file not found", 404
            content = read_tail(path, filter_text)
        except ValueError as error:
            return str(error), 400
        except OSError as error:
            return str(error), 404
    return render_template_string(PAGE, root=ROOT, files=log_files(), selected=selected,
                                  filter_text=filter_text, content=content)


@app.get("/api/logs")
def api_logs():
    return jsonify({"root": ROOT, "files": log_files()})


if __name__ == "__main__":
    serve(app, PORT, "Log viewer")
