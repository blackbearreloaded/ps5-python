class Counter:
    created = 0

    def __init__(self, value):
        self.value = value
        Counter.created += 1

    def increment(self, amount=1):
        self.value += amount
        return self.value


first = Counter(10)
second = Counter(3)
assert first.value == 10
assert second.value == 3
assert first.increment() == 11
assert first.increment(4) == 15
assert second.increment(2) == 5
assert Counter.created == 2
assert first is not second

print("classes_basic: PASS")
