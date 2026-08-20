import os
import sqlite3
import sys

from flask import Flask, jsonify, redirect, render_template_string, request, url_for

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".shared"))
from serve import serve


app = Flask("sqlite_notes")
PORT = 9103
DB_CANDIDATES = [
    os.environ.get("PYTHON_PS5_NOTES_DB", "/download0/python/notes.db"),
    "/data/python/notes.db",
]
DB_PATH = None

PAGE = """<!doctype html>
<title>SQLite Notes</title>
<style>
body{background:#101516;color:#edf0e3;font:15px system-ui;margin:30px;max-width:900px}
h1{color:#c7f36b}form,article{background:#182021;border:1px solid #344342;padding:16px;margin:12px 0}
input,textarea{display:block;width:100%;box-sizing:border-box;background:#0c1213;color:#edf0e3;border:1px solid #344342;padding:9px;margin:7px 0}
button{background:#c7f36b;border:0;padding:9px 14px;cursor:pointer}small{color:#879492}
</style>
<h1>SQLite Notes</h1><p>Database: <code>{{ db_path }}</code></p>
<form method="post" action="/notes"><input name="title" placeholder="Title" required><textarea name="body" rows="4" placeholder="Write a note" required></textarea><button>Add note</button></form>
{% for note in notes %}<article><h2>{{ note.title }}</h2><p>{{ note.body }}</p><small>{{ note.created }}</small></article>{% else %}<p>No notes yet.</p>{% endfor %}
"""


def initialize():
    global DB_PATH
    for candidate in DB_CANDIDATES:
        try:
            os.makedirs(os.path.dirname(candidate), exist_ok=True)
            with sqlite3.connect(candidate) as connection:
                connection.execute("CREATE TABLE IF NOT EXISTS notes (id INTEGER PRIMARY KEY, title TEXT NOT NULL, body TEXT NOT NULL, created TEXT NOT NULL)")
            DB_PATH = candidate
            return
        except OSError:
            continue
    raise RuntimeError("no writable notes directory")


def notes():
    with sqlite3.connect(DB_PATH) as connection:
        connection.row_factory = sqlite3.Row
        return [dict(row) for row in connection.execute("SELECT title, body, created FROM notes ORDER BY id DESC")]


@app.get("/")
def index():
    return render_template_string(PAGE, db_path=DB_PATH, notes=notes())


@app.post("/notes")
def add_note():
    title = request.form.get("title", "").strip()
    body = request.form.get("body", "").strip()
    if title and body:
        with sqlite3.connect(DB_PATH) as connection:
            connection.execute("INSERT INTO notes(title, body, created) VALUES (?, ?, datetime('now'))", (title, body))
    return redirect(url_for("index"))


@app.get("/api/notes")
def api_notes():
    return jsonify(notes())


if __name__ == "__main__":
    initialize()
    serve(app, PORT, "SQLite notes")
