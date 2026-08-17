"""Import-free generator-expression validation for CPythonPS5."""


source = [1, 2, 3, 4]
expression = (value * value for value in source)
assert expression.__class__.__name__ == "generator"
assert next(expression) == 1
assert next(expression) == 4
assert list(expression) == [9, 16]

value = "outer"
assert list(value * 2 for value in (1, 2, 3)) == [2, 4, 6]
assert value == "outer"

filtered = (number for number in range(10) if number % 2 == 0)
assert next(filtered) == 0
assert list(filtered) == [2, 4, 6, 8]

cross_product = ((left, right) for left in (1, 2) for right in ("a", "b"))
assert list(cross_product) == [(1, "a"), (1, "b"), (2, "a"), (2, "b")]

nested = (inner for outer in ((1, 2), (3,), ()) for inner in outer)
assert list(nested) == [1, 2, 3]

assert sum(number for number in range(1, 6)) == 15
assert any(number > 3 for number in (1, 2, 4))
assert not any(number < 0 for number in (1, 2, 4))
assert all(number % 2 == 0 for number in (2, 4, 6))
assert not all(number % 2 == 0 for number in (2, 3, 6))

rows = (("Ada", 10), ("Grace", 20), ("Linus", 30))
names = (name for name, score in rows if score >= 20)
assert list(names) == ["Grace", "Linus"]

assert list((number + 1 for number in [])) == []
assert list((number for number in range(3))) == [0, 1, 2]

print("generator_expressions: PASS")
