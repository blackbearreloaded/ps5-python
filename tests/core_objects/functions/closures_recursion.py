"""Closures, independent captured state, and recursive calls."""


def make_multiplier(factor):
    def multiply(value):
        return value * factor

    return multiply


double = make_multiplier(2)
triple = make_multiplier(3)
assert double(7) == 14
assert triple(7) == 21
assert double.__closure__[0].cell_contents == 2
assert triple.__closure__[0].cell_contents == 3


def factorial(value):
    if value <= 1:
        return 1
    return value * factorial(value - 1)


def fibonacci(value):
    if value < 2:
        return value
    return fibonacci(value - 1) + fibonacci(value - 2)


assert factorial(0) == 1
assert factorial(6) == 720
assert fibonacci(0) == 0
assert fibonacci(10) == 55


def make_recursive_sum():
    def sum_to(value):
        return 0 if value == 0 else value + sum_to(value - 1)

    return sum_to


assert make_recursive_sum()(100) == 5050

print("PASS: closures and recursion")
