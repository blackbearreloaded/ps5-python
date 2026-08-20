import html
import os
import re
import sys

from flask import Flask, jsonify, render_template_string, request

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".shared"))
from serve import serve


app = Flask("markdown_server")
PORT = 9107
DEFAULT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "assets"))
ROOT = os.path.abspath(os.environ.get("PYTHON_PS5_DOCS_ROOT", "/data/python/docs"))
if not os.path.isdir(ROOT):
    ROOT = DEFAULT_ROOT

PAGE = """<!doctype html>
<title>Markdown Server</title>
<style>
body{background:#101516;color:#edf0e3;font:16px system-ui;margin:30px;max-width:1150px}
h1,h2,h3{color:#c7f36b}.layout{display:grid;grid-template-columns:260px 1fr;gap:20px}
aside,article{background:#182021;border:1px solid #344342;padding:18px}a{color:#c7f36b}
pre{background:#0c1213;padding:14px;overflow:auto}code{color:#a8d4ff}.muted{color:#879492}
</style>
<h1>Markdown Server</h1><p class="muted">Browse lightweight notes and documentation from {{ root }}.</p>
<div class="layout"><aside><h2>Documents</h2><ul>{% for item in files %}<li><a href="/?file={{ item|urlencode }}">{{ item }}</a></li>{% endfor %}</ul></aside><article>{{ rendered|safe }}</article></div>
"""


def safe_path(relative):
    candidate = os.path.realpath(os.path.join(ROOT, relative or ""))
    if candidate != ROOT and not candidate.startswith(ROOT + os.sep):
        raise ValueError("path is outside the documents root")
    return candidate


def markdown_files():
    return sorted(name for name in os.listdir(ROOT) if name.lower().endswith((".md", ".markdown")))


def render_markdown(source):
    output = []
    in_code = False
    paragraph = []
    list_open = False

    def close_paragraph():
        nonlocal paragraph
        if paragraph:
            output.append("<p>{}</p>".format("<br>".join(paragraph)))
            paragraph = []

    def close_list():
        nonlocal list_open
        if list_open:
            output.append("</ul>")
            list_open = False

    for raw_line in source.splitlines():
        line = raw_line.rstrip()
        if line.startswith("```"):
            close_paragraph()
            close_list()
            if in_code:
                output.append("</code></pre>")
            else:
                output.append("<pre><code>")
            in_code = not in_code
            continue
        if in_code:
            output.append(html.escape(line))
            continue
        heading = re.match(r"^(#{1,3})\s+(.*)$", line)
        if heading:
            close_paragraph()
            close_list()
            level = len(heading.group(1))
            output.append("<h{0}>{1}</h{0}>".format(level, html.escape(heading.group(2))))
        elif line.startswith("- ") or line.startswith("* "):
            close_paragraph()
            if not list_open:
                output.append("<ul>")
                list_open = True
            output.append("<li>{}</li>".format(html.escape(line[2:])))
        elif not line:
            close_paragraph()
            close_list()
        else:
            close_list()
            paragraph.append(html.escape(line))
    close_paragraph()
    close_list()
    if in_code:
        output.append("</code></pre>")
    return "\n".join(output)


@app.get("/")
def index():
    selected = request.args.get("file", "").strip()
    files = markdown_files()
    if not selected and files:
        selected = files[0]
    try:
        path = safe_path(selected)
        if selected and (not os.path.isfile(path) or not selected.lower().endswith((".md", ".markdown"))):
            return "document not found", 404
        source = open(path, "r", encoding="utf-8", errors="replace").read() if selected else "No Markdown documents found."
        rendered = render_markdown(source)
        return render_template_string(PAGE, root=ROOT, files=files, rendered=rendered)
    except ValueError as error:
        return str(error), 400
    except OSError as error:
        return str(error), 404


@app.get("/api/files")
def api_files():
    return jsonify({"root": ROOT, "files": markdown_files()})


if __name__ == "__main__":
    serve(app, PORT, "Markdown server")
