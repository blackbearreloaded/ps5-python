import os
import socket
import ssl
import sys
import urllib.error
import urllib.parse
import urllib.request

from flask import Flask, jsonify, render_template_string, request

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".shared"))
from serve import serve


app = Flask("network_toolbox")
PORT = 9104

PAGE = """<!doctype html>
<title>Network Toolbox</title>
<style>
body{background:#101516;color:#edf0e3;font:15px system-ui;margin:30px;max-width:950px}
h1{color:#c7f36b}section{background:#182021;border:1px solid #344342;padding:16px;margin:14px 0}
input{background:#0c1213;color:#edf0e3;border:1px solid #344342;padding:9px;margin:4px;width:260px}
button{background:#c7f36b;border:0;padding:10px 14px;cursor:pointer}pre{white-space:pre-wrap;color:#b8c7c2}
.muted{color:#879492}
</style>
<h1>Network Toolbox</h1><p class="muted">Small, explicit DNS, TCP, and HTTP checks from the PS5 runtime.</p>
<section><h2>DNS and TCP</h2><form id="tcp"><input name="host" placeholder="microsoft.com" required><input name="port" type="number" value="443" min="1" max="65535"><button>Check host</button></form><pre id="tcp-result"></pre></section>
<section><h2>HTTP</h2><form id="http"><input name="url" value="https://example.com" required><button>Fetch URL</button></form><pre id="http-result"></pre></section>
<script>
async function submit(form, target, endpoint){
  const params=new URLSearchParams(new FormData(form));
  const response=await fetch(endpoint+'?'+params);
  document.querySelector(target).textContent=JSON.stringify(await response.json(), null, 2);
}
document.querySelector('#tcp').onsubmit=e=>{e.preventDefault();submit(e.target,'#tcp-result','/api/check')};
document.querySelector('#http').onsubmit=e=>{e.preventDefault();submit(e.target,'#http-result','/api/http')};
</script>
"""


@app.get("/")
def index():
    return render_template_string(PAGE)


@app.get("/api/check")
def check_host():
    host = request.args.get("host", "").strip()
    try:
        port = int(request.args.get("port", "443"))
    except ValueError:
        return jsonify({"error": "port must be a number"}), 400
    if not host or not 1 <= port <= 65535:
        return jsonify({"error": "host and valid port are required"}), 400

    addresses = []
    try:
        for result in socket.getaddrinfo(host, port, socket.AF_INET, socket.SOCK_STREAM):
            address = result[4][0]
            if address not in addresses:
                addresses.append(address)
    except OSError as error:
        return jsonify({"host": host, "port": port, "addresses": [], "error": str(error)}), 502

    tcp = {"ok": False}
    try:
        with socket.create_connection((host, port), timeout=3):
            tcp = {"ok": True}
    except OSError as error:
        tcp = {"ok": False, "error": str(error)}
    return jsonify({"host": host, "port": port, "addresses": addresses, "tcp": tcp})


@app.get("/api/http")
def check_http():
    url = request.args.get("url", "").strip()
    parsed = urllib.parse.urlparse(url)
    if parsed.scheme not in ("http", "https") or not parsed.netloc:
        return jsonify({"error": "use a complete http:// or https:// URL"}), 400
    def result(response, tls_verified):
        return {
            "ok": True,
            "status": response.status,
            "url": response.geturl(),
            "content_type": response.headers.get("Content-Type", ""),
            "server": response.headers.get("Server", ""),
            "tls_verified": tls_verified,
        }

    try:
        context = ssl.create_default_context() if parsed.scheme == "https" else None
        with urllib.request.urlopen(url, timeout=5, context=context) as response:
            return jsonify(result(response, True if parsed.scheme == "https" else None))
    except (OSError, urllib.error.URLError) as error:
        certificate_error = "CERTIFICATE_VERIFY_FAILED" in str(error)
        if parsed.scheme == "https" and certificate_error:
            try:
                with urllib.request.urlopen(
                    url, timeout=5, context=ssl._create_unverified_context()
                ) as response:
                    payload = result(response, False)
                    payload["tls_note"] = "PS5 CA bundle unavailable; certificate was not verified"
                    return jsonify(payload)
            except (OSError, urllib.error.URLError) as fallback_error:
                error = fallback_error
        return jsonify({"ok": False, "url": url, "error": str(error)}), 502


if __name__ == "__main__":
    serve(app, PORT, "Network toolbox")
