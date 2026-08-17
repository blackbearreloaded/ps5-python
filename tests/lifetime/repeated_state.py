"""Bounded repeated allocation and release test for Phase 4."""


class Sample:
    created = 0
    released = 0

    def __init__(self, value):
        self.value = value
        Sample.created += 1

    def __del__(self):
        Sample.released += 1


def run_round(round_number, count):
    values = []
    checksum = 0
    for index in range(count):
        value = round_number * count + index
        values.append((Sample(value), [value, value ^ 0x55], {"value": value}))
        checksum += value

    assert len(values) == count
    assert values[0][0].value == round_number * count
    assert values[-1][0].value == round_number * count + count - 1
    values = None
    return checksum


rounds = 64
count = 256
total = 0
for round_number in range(rounds):
    total += run_round(round_number, count)
    assert Sample.released == Sample.created

assert total == sum(round_number * count + index
                    for round_number in range(rounds)
                    for index in range(count))
assert Sample.created == rounds * count
assert Sample.released == Sample.created
print("lifetime/repeated_state.py: PASS")
