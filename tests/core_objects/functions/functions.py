"""Core function calls, defaults, and argument binding."""


def add(left, right=10):
    return left + right


def describe(name, greeting="hello", *, punctuation="!"):
    return greeting + ", " + name + punctuation


def collect(first, *rest, **options):
    return first, rest, options


assert add(2) == 12
assert add(2, 3) == 5
assert describe("PS5") == "hello, PS5!"
assert describe("PS5", punctuation=".") == "hello, PS5."
assert collect("a", "b", "c", mode="test") == (
    "a", ("b", "c"), {"mode": "test"}
)


def make_counter(start=0):
    value = start

    def next_value(step=1):
        nonlocal value
        value += step
        return value

    return next_value


counter = make_counter(4)
assert counter() == 5
assert counter(3) == 8

print("PASS: functions and default arguments")
