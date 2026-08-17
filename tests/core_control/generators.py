"""Import-free generator protocol validation for CPythonPS5."""


events = []


def lazy_values():
    events.append("started")
    yield 10
    events.append("resumed")
    yield 20
    events.append("finished")


generator = lazy_values()
assert events == []
assert next(generator) == 10
assert events == ["started"]
assert next(generator) == 20
assert events == ["started", "resumed"]

try:
    next(generator)
except StopIteration as error:
    assert error.value is None
else:
    assert False

assert events == ["started", "resumed", "finished"]
try:
    next(generator)
except StopIteration:
    pass
else:
    assert False


def countdown(start):
    while start:
        received = yield start
        if received is None:
            start -= 1
        else:
            start = received
    return "complete"


counter = countdown(3)
assert next(counter) == 3
assert counter.send(None) == 2
assert counter.send(5) == 5
assert counter.send(1) == 1
try:
    counter.send(None)
except StopIteration as error:
    assert error.value == "complete"
else:
    assert False


def delegated():
    result = yield from (value * 2 for value in (1, 2, 3))
    yield result


assert list(delegated()) == [2, 4, 6, None]


def returned_value():
    yield "before return"
    return 99


returned = returned_value()
assert next(returned) == "before return"
try:
    next(returned)
except StopIteration as error:
    assert error.value == 99
else:
    assert False

print("generators: PASS")
