"""Import-free iterator and for-loop validation for CPythonPS5."""


items = ["a", "b", "c"]
iterator = iter(items)
assert next(iterator) == "a"
assert next(iterator) == "b"
assert next(iterator) == "c"
assert next(iterator, "done") == "done"
assert next(iterator, "still done") == "still done"
assert iter(iterator) is iterator

seen = []
for item in (1, 2, 3, 4):
    seen.append(item * 10)
assert seen == [10, 20, 30, 40]

selected = []
for number in range(7):
    if number % 2 == 0:
        continue
    selected.append(number)
    if number == 5:
        break
assert selected == [1, 3, 5]

completed = False
for number in range(3):
    assert number < 3
else:
    completed = True
assert completed

completed = False
for number in range(3):
    if number == 1:
        break
else:
    completed = False
assert not completed


class CountIterator:
    def __init__(self, limit):
        self.current = 0
        self.limit = limit

    def __iter__(self):
        return self

    def __next__(self):
        if self.current >= self.limit:
            raise StopIteration
        value = self.current
        self.current += 1
        return value


count = CountIterator(4)
assert iter(count) is count
assert list(count) == [0, 1, 2, 3]
assert list(count) == []
assert next(count, "finished") == "finished"

characters = []
for character in "PS5":
    characters.append(character)
assert characters == ["P", "S", "5"]

keys = []
for key in {"first": 1, "second": 2}:
    keys.append(key)
assert set(keys) == {"first", "second"}

calls = []


def next_value():
    calls.append(len(calls))
    return calls[-1]


for value in iter(next_value, 3):
    assert value < 3
assert calls == [0, 1, 2, 3]

print("iterators: PASS")
