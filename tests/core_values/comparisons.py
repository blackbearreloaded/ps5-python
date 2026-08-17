"""Import-free checks for Python comparison and boolean expressions."""

assert 3 == 3
assert 3 != 4
assert 2 < 3 <= 3 < 4
assert 4 > 3 >= 3 > 2
assert not (5 < 2)

left = 10
right = 10
other = 11
assert left == right
assert left is right
assert left is not other

assert (True and True) is True
assert (True and False) is False
assert (False or True) is True
assert (not False) is True
assert (0 or 7) == 7
assert (9 and 4) == 4

assert "beta" in ("alpha", "beta", "gamma")
assert "delta" not in ("alpha", "beta", "gamma")
assert 3 in [1, 2, 3]
assert 8 not in [1, 2, 3]

assert (1 < 2) == (3 <= 3)
assert (4 == 4) != (4 == 5)

print("PASS: comparison core values")
