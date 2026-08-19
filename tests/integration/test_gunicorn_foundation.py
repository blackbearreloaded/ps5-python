"""Bounded Gunicorn-style arbiter/worker checks.

The assertions are adapted from the pinned CPython 3.14.7 tests for ``os``,
``signal``, and ``socket``.  They exercise the small process boundary that a
pre-fork WSGI server needs without importing third-party Gunicorn code.
"""

import os
import signal
import socket
import sys


if not hasattr(os, "fork") or not sys.platform.startswith("freebsd"):
    print("test_gunicorn_foundation: SKIP (PS5 fork boundary only)")
    raise SystemExit(0)


def wait_for_exit(pid):
    """Reap one worker and require a normal zero exit status."""
    child_pid, status = os.waitpid(pid, 0)
    assert child_pid == pid
    assert os.WIFEXITED(status)
    assert os.WEXITSTATUS(status) == 0


def run_inherited_socket_worker(listener_fd, address, marker):
    """Fork one worker which accepts one request on the inherited listener."""
    pid = os.fork()
    if pid == 0:
        try:
            # Gunicorn workers inherit the arbiter's listening fd.  Reuse it
            # directly: PS5 does not provide dup()/fromfd(), and no duplicate
            # descriptor is needed for a forked worker.
            worker_listener = socket.socket(
                socket.AF_INET, socket.SOCK_STREAM, fileno=listener_fd
            )
            connection, _ = worker_listener.accept()
            try:
                request = connection.recv(64)
                assert request == b"GET / HTTP/1.0\r\n\r\n"
                connection.sendall(marker)
            finally:
                connection.close()
                worker_listener.close()
        except BaseException:
            os._exit(1)
        os._exit(0)
    return pid


# Arbiter socket inheritance and two independent pre-fork workers.  Each
# worker handles one request and exits, which keeps the test deterministic and
# avoids a long-running server in the aggregate suite.
listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.bind(("127.0.0.1", 0))
listener.listen(4)
address = listener.getsockname()

worker_pids = [
    run_inherited_socket_worker(listener.fileno(), address, b"worker-one"),
    run_inherited_socket_worker(listener.fileno(), address, b"worker-two"),
]
listener.close()

responses = []
for _ in worker_pids:
    client = socket.create_connection(address, timeout=2.0)
    try:
        client.sendall(b"GET / HTTP/1.0\r\n\r\n")
        responses.append(client.recv(64))
    finally:
        client.close()

for pid in worker_pids:
    wait_for_exit(pid)

assert sorted(responses) == [b"worker-one", b"worker-two"]
print("test_gunicorn_foundation: inherited listener and pre-fork workers PASS")


# The arbiter must be able to terminate and reap a stuck worker.  A blocked
# pipe read gives the child a deterministic wait state without relying on
# time.sleep(), whose PS5 libc hook is intentionally not part of this test.
read_fd, write_fd = os.pipe()
pid = os.fork()
if pid == 0:
    try:
        os.close(write_fd)
        os.read(read_fd, 1)
    finally:
        os._exit(0)

os.close(read_fd)
os.close(write_fd)
try:
    os.kill(pid, signal.SIGTERM)
    child_pid, status = os.waitpid(pid, 0)
    assert child_pid == pid
    assert os.WIFSIGNALED(status)
    assert os.WTERMSIG(status) == signal.SIGTERM
finally:
    # The normal path has already reaped the worker.  Do not leave a child if
    # a platform-specific wait status assertion fails.
    try:
        os.kill(pid, signal.SIGKILL)
    except OSError:
        pass
    try:
        os.waitpid(pid, os.WNOHANG)
    except OSError:
        pass

previous_sigchld = signal.getsignal(signal.SIGCHLD)


def sigchld_handler(signum, frame):
    return None


signal.signal(signal.SIGCHLD, sigchld_handler)
assert signal.getsignal(signal.SIGCHLD) is sigchld_handler
signal.signal(signal.SIGCHLD, previous_sigchld)
assert signal.getsignal(signal.SIGCHLD) is previous_sigchld

print("test_gunicorn_foundation: SIGTERM, waitpid, and SIGCHLD PASS")
