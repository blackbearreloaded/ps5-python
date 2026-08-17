"""Import-free checks for Python's core numeric operations."""

assert 7 + 5 == 12
assert 9 - 14 == -5
assert 6 * 7 == 42
assert 20 // 6 == 3
assert 20 % 6 == 2
assert 2 ** 10 == 1024
assert -(-8) == 8

assert 0.25 + 0.75 == 1.0
assert 7 / 2 == 3.5
assert abs(-3.5) == 3.5
assert round(3.14159, 2) == 3.14

assert (1 << 5) == 32
assert (0b1101 & 0b1011) == 0b1001
assert (0b1101 | 0b0010) == 0b1111
assert (0b1101 ^ 0b1011) == 0b0110
assert (~0b0011) == -4

assert int(3.9) == 3
assert float(4) == 4.0
assert bool(0) is False
assert bool(1) is True

print("PASS: numeric core values")
