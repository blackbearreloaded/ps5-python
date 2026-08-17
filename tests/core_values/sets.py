"""Import-free checks for Python set values and operations."""


features = {"cpu", "files", "network"}
assert len(features) == 3
assert "cpu" in features
assert "gpu" not in features

features.add("graphics")
assert "graphics" in features
features.remove("network")
assert "network" not in features
features.discard("missing")

left = {1, 2, 3, 4}
right = {3, 4, 5}
assert left | right == {1, 2, 3, 4, 5}
assert left & right == {3, 4}
assert left - right == {1, 2}
assert right - left == {5}
assert left ^ right == {1, 2, 5}
assert {1, 2} <= left
assert left >= {1, 2}

assert {number * 2 for number in range(4)} == {0, 2, 4, 6}
assert len(set([1, 1, 2, 3, 3])) == 3

print("PASS: set core values")
