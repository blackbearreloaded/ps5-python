"""Bounded recursion, exception, and finally-cleanup checks."""


def recurse_until_limit(value):
    return recurse_until_limit(value + 1)


recursion_seen = False
try:
    recurse_until_limit(0)
except RecursionError:
    recursion_seen = True
finally:
    recursion_finished = True

assert recursion_seen
assert recursion_finished

events = []
try:
    events.append("try")
    raise RuntimeError("expected lifetime error")
except RuntimeError as error:
    events.append(str(error))
finally:
    events.append("finally")

assert events == ["try", "expected lifetime error", "finally"]
print("lifetime/recursion_and_errors.py: PASS")
