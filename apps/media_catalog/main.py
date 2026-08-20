import os
import sys
import time

from flask import Flask, jsonify, render_template_string, request

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".shared"))
from serve import serve


app = Flask("media_catalog")
PORT = 9110
ROOT = os.path.abspath(os.environ.get("PYTHON_PS5_MEDIA_ROOT", "/data"))
CATEGORIES = {
    "images": {".jpg", ".jpeg", ".png", ".gif", ".webp", ".bmp"},
    "video": {".mp4", ".mkv", ".webm", ".avi", ".mov"},
    "audio": {".mp3", ".wav", ".ogg", ".flac", ".m4a"},
    "documents": {".pdf", ".txt", ".md", ".doc", ".docx", ".json", ".csv"},
    "archives": {".zip", ".tar", ".gz", ".7z", ".rar"},
}

PAGE = """<!doctype html>
<title>Media Catalog</title>
<style>
body{background:#101516;color:#edf0e3;font:15px system-ui;margin:30px;max-width:1200px}
h1{color:#c7f36b}.filters{display:flex;gap:8px;flex-wrap:wrap;margin:15px 0}.filters a{border:1px solid #344342;padding:8px 11px;color:#c7f36b;text-decoration:none}
input{background:#0c1213;color:#edf0e3;border:1px solid #344342;padding:8px}button{background:#c7f36b;border:0;padding:9px 14px}
table{border-collapse:collapse;width:100%;background:#182021}th,td{padding:9px;border-bottom:1px solid #344342;text-align:left}th{color:#c7f36b}.muted{color:#879492}
</style>
<h1>Media Catalog</h1><p class="muted">Read-only inventory under {{ root }} · {{ items|length }} matching files{% if limited %} (first 5000 scanned){% endif %}.</p>
<div class="filters">{% for value in types %}<a href="/?type={{ value }}">{{ value }}</a>{% endfor %}</div>
<form><input name="q" value="{{ query }}" placeholder="search filename"><input type="hidden" name="type" value="{{ selected_type }}"><button>Filter</button></form>
<table><tr><th>File</th><th>Type</th><th>Size</th><th>Modified</th></tr>{% for item in items %}<tr><td>{{ item.path }}</td><td>{{ item.category }}</td><td>{{ item.size }}</td><td>{{ item.modified }}</td></tr>{% else %}<tr><td colspan="4">No matching files.</td></tr>{% endfor %}</table>
"""


def display_size(size):
    units = ("B", "KB", "MB", "GB")
    value = float(size)
    for unit in units:
        if value < 1024 or unit == units[-1]:
            return "{0:.1f} {1}".format(value, unit) if unit != "B" else "{0} B".format(size)
        value /= 1024


def category_for(name):
    extension = os.path.splitext(name)[1].lower()
    for category, extensions in CATEGORIES.items():
        if extension in extensions:
            return category
    return "other"


def catalog():
    result = []
    scanned = 0
    for current, directories, names in os.walk(ROOT):
        directories[:] = [name for name in directories if not name.startswith(".")]
        for name in names:
            scanned += 1
            if scanned > 5000:
                return result, True
            path = os.path.join(current, name)
            try:
                stat = os.stat(path)
                result.append({
                    "path": os.path.relpath(path, ROOT).replace("\\", "/"),
                    "category": category_for(name),
                    "size": display_size(stat.st_size),
                    "modified": time.strftime("%Y-%m-%d %H:%M", time.localtime(stat.st_mtime)),
                })
            except OSError:
                continue
    return result, False


def filtered_items(selected_type, query):
    items, limited = catalog()
    if selected_type != "all":
        items = [item for item in items if item["category"] == selected_type]
    if query:
        needle = query.lower()
        items = [item for item in items if needle in item["path"].lower()]
    return sorted(items, key=lambda item: item["path"].lower()), limited


@app.get("/")
def index():
    selected_type = request.args.get("type", "all").strip().lower()
    if selected_type not in set(CATEGORIES) | {"all", "other"}:
        selected_type = "all"
    query = request.args.get("q", "").strip()
    items, limited = filtered_items(selected_type, query)
    return render_template_string(PAGE, root=ROOT, items=items, limited=limited,
                                  selected_type=selected_type, query=query,
                                  types=("all",) + tuple(CATEGORIES) + ("other",))


@app.get("/api/files")
def api_files():
    selected_type = request.args.get("type", "all").strip().lower()
    query = request.args.get("q", "").strip()
    items, limited = filtered_items(selected_type, query)
    return jsonify({"root": ROOT, "limited": limited, "items": items})


if __name__ == "__main__":
    serve(app, PORT, "Media catalog")
