"""Import-free try/finally smoke test for CPythonPS5."""


events = []

try:
    events.append("try")
finally:
    events.append("finally")

assert events == ["try", "finally"]


def return_with_cleanup():
    events = ["try"]
    try:
        return events
    finally:
        events.append("finally")


assert return_with_cleanup() == ["try", "finally"]

events = []
try:
    events.append("try")
    raise RuntimeError("expected")
except RuntimeError:
    events.append("except")
finally:
    events.append("finally")

assert events == ["try", "except", "finally"]

print("core_control/try_finally.py: PASS")
