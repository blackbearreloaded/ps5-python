"""Import-free cyclic-garbage-collection stress test."""


class CycleNode:
    created = 0
    finalized = 0
    finalized_sum = 0

    def __init__(self, value):
        self.value = value
        self.peer = None
        self.payload = [value, value ^ 85]
        CycleNode.created += 1

    def __del__(self):
        CycleNode.finalized += 1
        CycleNode.finalized_sum += self.value


def create_cycles(count):
    expected_sum = 0
    for index in range(count):
        first = CycleNode(index)
        second = CycleNode(index + count)
        first.peer = second
        second.peer = first
        assert first.peer is second
        assert second.peer is first
        assert first.payload[0] == index
        assert second.payload[0] == index + count
        expected_sum += index + index + count
        first = None
        second = None

        noise = []
        for noise_value in range(8):
            noise.append({"value": noise_value, "items": [index, noise_value]})
        assert len(noise) == 8
        noise = None

    first = None
    second = None
    return expected_sum


cycle_count = 4096
expected_sum = create_cycles(cycle_count)
assert CycleNode.created == cycle_count * 2
assert CycleNode.finalized > 0
assert CycleNode.finalized <= CycleNode.created
assert CycleNode.finalized_sum >= 0

for round_number in range(32):
    pressure = []
    for value in range(256):
        pressure.append([round_number, value, {"value": value}])
    assert len(pressure) == 256
    assert pressure[0][0] == round_number
    assert pressure[-1][1] == 255
    pressure = None

assert CycleNode.finalized > 0
assert CycleNode.finalized <= CycleNode.created
assert expected_sum == cycle_count * (cycle_count * 2 - 1)
print("core_control/gc_stress.py: PASS")
