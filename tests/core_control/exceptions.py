"""Import-free exception-control smoke test for Python-PS5."""


events = []

try:
    events.append("try")
    raise ValueError("expected")
except TypeError:
    events.append("wrong-handler")
except ValueError as error:
    events.append("value-handler")
    assert str(error) == "expected"
else:
    events.append("else")
finally:
    events.append("finally")

assert events == ["try", "value-handler", "finally"]

try:
    raise KeyError("missing")
except LookupError as error:
    assert error.args == ("missing",)
else:
    assert False

print("core_control/exceptions.py: PASS")
