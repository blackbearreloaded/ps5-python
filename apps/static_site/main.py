import os
import sys
import time

from flask import Flask, jsonify, send_from_directory

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".shared"))
from serve import serve


app = Flask("static_site")
PORT = 9109
ASSET_ROOT = os.path.join(os.path.dirname(__file__), "assets")


@app.get("/")
def index():
    return send_from_directory(ASSET_ROOT, "index.html")


@app.get("/assets/<path:name>")
def assets(name):
    return send_from_directory(ASSET_ROOT, name)


@app.get("/api/time")
def api_time():
    return jsonify({"unix": time.time(), "service": "static_site"})


if __name__ == "__main__":
    serve(app, PORT, "Static site")
