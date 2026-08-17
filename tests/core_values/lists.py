"""Import-free checks for Python list values and operations."""


values = [1, 2, 3]
assert len(values) == 3
assert values[0] == 1
assert values[-1] == 3
assert values[1:] == [2, 3]

values.append(4)
assert values == [1, 2, 3, 4]
values.extend([5, 6])
assert values == [1, 2, 3, 4, 5, 6]
values[0] = 10
assert values[0] == 10
values[1:3] = [20, 30]
assert values[:3] == [10, 20, 30]
assert 30 in values
assert 99 not in values

assert [number * 2 for number in range(4)] == [0, 2, 4, 6]
assert list(reversed([1, 2, 3])) == [3, 2, 1]
assert sorted([3, 1, 2]) == [1, 2, 3]

print("PASS: list core values")
