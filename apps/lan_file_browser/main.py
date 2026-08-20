import os
import sys
import time

from flask import Flask, jsonify, render_template_string, request, send_file

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".shared"))
from serve import serve


app = Flask("lan_file_browser")
PORT = 9105
ROOT = os.path.abspath(os.environ.get("CPYTHONPS5_SHARE_ROOT", "/"))
try:
    os.makedirs(ROOT, exist_ok=True)
except OSError:
    ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "share"))
    os.makedirs(ROOT, exist_ok=True)

PAGE = """<!doctype html>
<title>LAN File Browser</title>
<style>
body{background:#101516;color:#edf0e3;font:15px system-ui;margin:30px;max-width:1100px}
h1{color:#c7f36b}.path{font-family:monospace;color:#879492}table{border-collapse:collapse;width:100%}
th,td{padding:9px;border-bottom:1px solid #344342;text-align:left}th{color:#c7f36b}
a{color:#c7f36b}.note{color:#879492}
</style>
<h1>LAN File Browser</h1><p class="path">Share root: {{ root }} · Current: {{ relative }}</p>
<p class="note">Read-only browser for files you place in the configured share directory.</p>
{% if parent is not none %}<p><a href="/?path={{ parent|urlencode }}">↑ Parent directory</a></p>{% endif %}
<table><tr><th>Name</th><th>Kind</th><th>Size</th><th>Modified</th><th>Action</th></tr>
{% for item in items %}<tr><td>{% if item.directory %}<a href="/?path={{ item.path|urlencode }}">{{ item.name }}/</a>{% else %}{{ item.name }}{% endif %}</td><td>{{ item.kind }}</td><td>{{ item.size }}</td><td>{{ item.modified }}</td><td>{% if not item.directory %}<a href="/download?path={{ item.path|urlencode }}">Download</a>{% endif %}</td></tr>{% endfor %}
</table>
"""


def safe_path(relative):
    candidate = os.path.realpath(os.path.join(ROOT, relative or ""))
    if os.path.commonpath((ROOT, candidate)) != ROOT:
        raise ValueError("path is outside the share root")
    return candidate


def display_size(size):
    units = ("B", "KB", "MB", "GB")
    value = float(size)
    for unit in units:
        if value < 1024 or unit == units[-1]:
            return "{0:.1f} {1}".format(value, unit) if unit != "B" else "{0} B".format(size)
        value /= 1024


def list_directory(path):
    items = []
    entries = sorted(os.scandir(path), key=lambda item: (not item.is_dir(follow_symlinks=False), item.name.lower()))
    for entry in entries:
        try:
            stat = entry.stat(follow_symlinks=False)
            directory = entry.is_dir(follow_symlinks=False)
            items.append({
                "name": entry.name,
                "path": os.path.relpath(entry.path, ROOT).replace("\\", "/"),
                "directory": directory,
                "kind": "directory" if directory else "file",
                "size": "—" if directory else display_size(stat.st_size),
                "modified": time.strftime("%Y-%m-%d %H:%M", time.localtime(stat.st_mtime)),
            })
        except OSError:
            continue
    return items


@app.get("/")
def index():
    relative = request.args.get("path", "").strip("/")
    try:
        current = safe_path(relative)
        if not os.path.isdir(current):
            return "directory not found", 404
        parent = os.path.dirname(relative).replace("\\", "/") if relative else None
        return render_template_string(PAGE, root=ROOT, relative=relative or ".",
                                      parent=parent, items=list_directory(current))
    except ValueError as error:
        return str(error), 400


@app.get("/api/list")
def api_list():
    relative = request.args.get("path", "").strip("/")
    try:
        return jsonify({"root": ROOT, "path": relative, "items": list_directory(safe_path(relative))})
    except ValueError as error:
        return jsonify({"error": str(error)}), 400
    except OSError:
        return jsonify({"error": "directory not found"}), 404


@app.get("/download")
def download():
    relative = request.args.get("path", "").strip("/")
    try:
        path = safe_path(relative)
        if not os.path.isfile(path):
            return "file not found", 404
        return send_file(path, as_attachment=True, download_name=os.path.basename(path))
    except ValueError as error:
        return str(error), 400


if __name__ == "__main__":
    serve(app, PORT, "LAN file browser")
