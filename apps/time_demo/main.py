import time
from _cpython_ps5_control import stop_requested


def wait_seconds(seconds):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if stop_requested():
            raise KeyboardInterrupt
        pass


for index in range(1, 4):
    print("Time demo: message {0}/3 at {1:.3f}".format(index, time.time()), flush=True)
    if index < 3:
        wait_seconds(5)

print("Time demo: finished", flush=True)
