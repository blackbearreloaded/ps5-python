import json
import os
import sys
import threading
import time
from collections import deque

from flask import Flask, jsonify, render_template_string, request

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".shared"))
from serve import serve


app = Flask("webhook_inspector")
PORT = 9108
EVENTS = deque(maxlen=25)
LOCK = threading.Lock()

PAGE = """<!doctype html>
<title>Webhook Inspector</title>
<style>
body{background:#101516;color:#edf0e3;font:15px system-ui;margin:30px;max-width:1100px}
h1{color:#c7f36b}section{background:#182021;border:1px solid #344342;padding:16px;margin:14px 0}
code,pre{background:#0c1213;padding:10px;white-space:pre-wrap;overflow:auto}a{color:#c7f36b}.muted{color:#879492}
</style>
<h1>Webhook Inspector</h1><p class="muted">POST JSON or text to <code>/hook</code>. The last 25 requests stay in memory.</p>
<section><h2>Try it</h2><pre>curl -X POST http://PS5-IP:9108/hook -H "Content-Type: application/json" -d '{"event":"demo"}'</pre></section>
<section><h2>Recent requests</h2><pre id="events">Loading…</pre></section>
<script>
async function refresh(){document.querySelector('#events').textContent=JSON.stringify(await (await fetch('/api/events')).json(),null,2)}
refresh();setInterval(refresh,2000);
</script>
"""


def recent_events():
    with LOCK:
        return list(EVENTS)


@app.get("/")
def index():
    return render_template_string(PAGE)


@app.post("/hook")
def hook():
    body = request.get_data(cache=True)[:8192]
    parsed = request.get_json(silent=True)
    event = {
        "received": time.strftime("%Y-%m-%d %H:%M:%S"),
        "remote": request.remote_addr,
        "content_type": request.content_type or "",
        "user_agent": request.headers.get("User-Agent", ""),
        "json": parsed,
        "body": body.decode("utf-8", "replace"),
    }
    with LOCK:
        EVENTS.appendleft(event)
    print("Webhook received: {0} bytes".format(len(body)), flush=True)
    return jsonify({"ok": True, "stored": len(EVENTS)})


@app.get("/api/events")
def api_events():
    return jsonify(recent_events())


if __name__ == "__main__":
    serve(app, PORT, "Webhook inspector")
