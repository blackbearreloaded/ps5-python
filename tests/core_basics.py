"""Import-free language-core smoke test for CPythonPS5."""


def check(condition, name):
    if not condition:
        raise AssertionError(name)


def make_adder(value):
    def add(other):
        return value + other

    return add


class Counter:
    def __init__(self, start):
        self.value = start

    def increment(self):
        self.value += 1
        return self.value


def numbers(limit):
    current = 0
    while current < limit:
        yield current
        current += 1


check(10 + 32 == 42, "arithmetic")
check([x * 2 for x in range(4)] == [0, 2, 4, 6], "comprehension")
check({"answer": 42}["answer"] == 42, "dictionary")
check(make_adder(10)(32) == 42, "closure")

counter = Counter(41)
check(counter.increment() == 42, "class method")

values = list(numbers(4))
check(values == [0, 1, 2, 3], "generator")

try:
    raise ValueError("expected")
except ValueError as error:
    check(str(error) == "expected", "exception")

print("CPYTHON_CORE_TESTS: PASS")
