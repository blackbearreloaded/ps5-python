from _cpython_ps5_control import stop_requested
from werkzeug.serving import make_server


def serve(app, port, name):
    # App concurrency comes from the process supervisor. Keeping each WSGI
    # server single-threaded avoids depending on the PS5 runtime's optional
    # Python worker-thread path while preserving responsive independent apps.
    server = make_server("0.0.0.0", port, app)
    server.timeout = 0.5
    print("{0} listening on 0.0.0.0:{1}".format(name, port), flush=True)
    try:
        while not stop_requested():
            server.handle_request()
    finally:
        server.server_close()
