"""Import-free checks for Python tuple values and operations."""


point = (10, 20, 30)
assert len(point) == 3
assert point[0] == 10
assert point[-1] == 30
assert point[1:] == (20, 30)
assert point + (40,) == (10, 20, 30, 40)
assert ("PS5",) * 2 == ("PS5", "PS5")
assert 20 in point
assert 99 not in point

first, second, third = point
assert (first, second, third) == (10, 20, 30)
assert (1, 2) == (1, 2)
assert (1, 2) != (2, 1)
assert tuple(number * 2 for number in range(3)) == (0, 2, 4)

print("PASS: tuple core values")
