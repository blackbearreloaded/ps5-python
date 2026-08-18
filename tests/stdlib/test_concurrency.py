"""Import-light adaptations of CPython threading and multiprocessing tests."""

import os
import sys
import threading

from concurrent.futures import ThreadPoolExecutor, as_completed

try:
    from concurrent.futures import ProcessPoolExecutor  # noqa: F401
except (ImportError, ModuleNotFoundError):
    print("test_concurrency: ProcessPoolExecutor unavailable")


# CPython Lib/test/test_threading.py and test_concurrent_futures.py.
main_ident = threading.get_ident()
started = threading.Event()
worker_idents = []


def record_worker(value):
    worker_idents.append(threading.get_ident())
    started.set()
    return value * value


with ThreadPoolExecutor(max_workers=2, thread_name_prefix="cpython-ps5") as pool:
    futures = [pool.submit(record_worker, value) for value in (2, 3, 4)]
    assert started.wait(2.0)
    assert sorted(future.result() for future in futures) == [4, 9, 16]
    assert sorted(pool.map(lambda value: value + 1, (1, 2, 3))) == [2, 3, 4]
    completed = list(as_completed(futures))
    assert len(completed) == 3

    failed = pool.submit(lambda: (_ for _ in ()).throw(ValueError("worker")))
    try:
        failed.result()
    except ValueError as error:
        assert str(error) == "worker"
    else:
        raise AssertionError("Future did not propagate worker exception")

assert worker_idents
assert all(ident != main_ident for ident in worker_idents)

event = threading.Event()
thread = threading.Thread(target=event.set, name="cpython-ps5-test")
thread.start()
thread.join(2.0)
assert not thread.is_alive()
assert event.is_set()

lock = threading.Lock()
with lock:
    assert lock.locked()
assert not lock.locked()

# PS5-specific IPC capability checks are kept separate from the deterministic
# host suite. The official multiprocessing package is bundled, but process
# launching and file-backed shared memory remain platform-limited.
if sys.platform.startswith("freebsd"):
    import multiprocessing

    def process_child(connection):
        connection.send(("process", os.getpid()))
        connection.close()

    assert multiprocessing.get_start_method() in ("fork", "forkserver", "spawn")
    assert multiprocessing.current_process().pid == os.getpid()
    assert multiprocessing.cpu_count() > 0

    receiver, sender = multiprocessing.Pipe(False)
    try:
        sender.send({"kind": "pipe", "value": 42})
        assert receiver.recv() == {"kind": "pipe", "value": 42}
    finally:
        receiver.close()
        sender.close()

    receiver, sender = multiprocessing.Pipe(False)
    process = multiprocessing.Process(target=process_child, args=(sender,))
    process.start()
    sender.close()
    assert receiver.recv()[0] == "process"
    receiver.close()
    process.join(2.0)
    assert process.exitcode == 0

    try:
        queue = multiprocessing.Queue()
    except (ImportError, OSError) as error:
        # The PS5 payload currently has no named semaphore filesystem.
        assert getattr(error, "errno", None) in (2, 38, 45, 95)
        print("test_concurrency: multiprocessing.Queue unavailable")
    else:
        try:
            queue.put(("queue", 42))
            assert queue.get(timeout=2.0) == ("queue", 42)
        finally:
            queue.close()
            queue.join_thread()

    try:
        semaphore = multiprocessing.Semaphore(1)
    except (ImportError, OSError) as error:
        assert getattr(error, "errno", None) in (2, 38, 45, 95)
        print("test_concurrency: multiprocessing.Semaphore unavailable")
    else:
        assert semaphore.acquire(False)
        semaphore.release()

    try:
        from multiprocessing.shared_memory import SharedMemory
    except ImportError:
        print("test_concurrency: SharedMemory unavailable")
    else:
        try:
            shared = SharedMemory(create=True, size=16)
        except OSError as error:
            # mmap is currently ENOTSUP in the PS5 payload.
            assert error.errno in (38, 45, 95)
            print("test_concurrency: SharedMemory unavailable (ENOTSUP)")
        else:
            try:
                shared.buf[:4] = b"PS5!"
                assert bytes(shared.buf[:4]) == b"PS5!"
            finally:
                shared.close()
                shared.unlink()
else:
    print("test_concurrency: multiprocessing IPC checks skipped on host")

print("test_concurrency: PASS")
