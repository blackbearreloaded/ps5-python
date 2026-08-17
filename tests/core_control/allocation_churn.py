"""Import-free allocation and automatic reference-counting stress test."""


class AllocationProbe:
    created = 0
    destroyed = 0
    destroyed_sum = 0

    def __init__(self, value):
        self.value = value
        AllocationProbe.created += 1

    def __del__(self):
        AllocationProbe.destroyed += 1
        AllocationProbe.destroyed_sum += self.value


def allocate_batch(count, start):
    batch = []
    expected_sum = 0

    for offset in range(count):
        value = start + offset
        expected_sum += value
        probe = AllocationProbe(value)
        batch.append((probe, {"value": value}, [value, value + 1]))

    assert len(batch) == count
    assert batch[0][0].value == start
    assert batch[-1][0].value == start + count - 1
    assert batch[0][1]["value"] == start
    assert batch[-1][2] == [start + count - 1, start + count]

    actual_sum = 0
    for probe, mapping, values in batch:
        assert probe.value == mapping["value"]
        assert values[0] == probe.value
        actual_sum += probe.value

    assert actual_sum == expected_sum
    batch = None
    probe = None
    mapping = None
    values = None
    return expected_sum


total = 0
batch_size = 256
batch_count = 32
for batch_number in range(batch_count):
    total += allocate_batch(batch_size, batch_number * batch_size)

assert AllocationProbe.created == batch_size * batch_count
assert AllocationProbe.destroyed == AllocationProbe.created
assert AllocationProbe.destroyed_sum == total

shared = {"items": []}
for value in range(512):
    shared["items"].append([value, value * 2, {"value": value}])
assert len(shared["items"]) == 512
assert shared["items"][0] == [0, 0, {"value": 0}]
assert shared["items"][-1] == [511, 1022, {"value": 511}]
shared = None

assert AllocationProbe.destroyed == AllocationProbe.created
print("core_control/allocation_churn.py: PASS")
