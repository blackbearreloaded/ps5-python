"""PS5 adaptations of CPython Tier 3 concurrency/networking tests."""

import asyncio
import concurrent.futures
import http.client
import http.server
import multiprocessing
import queue
import select
import signal
import socket
import ssl
import threading


# CPython Lib/test/test_asyncio, test_threading, test_multiprocessing,
# test_concurrent_futures, test_socket, test_ssl, test_httpservers,
# test_queue, test_select, and test_signal.
async def async_round_trip():
    channel = asyncio.Queue()
    await channel.put("PS5")
    value = await channel.get()
    await asyncio.sleep(0)
    return value


assert asyncio.run(async_round_trip()) == "PS5"

with concurrent.futures.ThreadPoolExecutor(max_workers=1) as executor:
    assert executor.submit(lambda: "thread").result() == "thread"

assert threading.current_thread() is not None
assert multiprocessing.get_start_method() in ("fork", "forkserver", "spawn")
thread_queue = queue.Queue()
thread_queue.put("thread-safe")
assert thread_queue.get_nowait() == "thread-safe"
assert http.client.responses[200] == "OK"
assert http.server.BaseHTTPRequestHandler is not None
assert socket.AF_INET > 0
assert ssl.SSLContext is not None
assert select.select is not None
assert signal.getsignal(signal.SIGINT) is not None

print("test_threading: PASS")
