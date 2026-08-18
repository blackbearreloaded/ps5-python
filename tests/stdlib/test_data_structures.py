"""PS5 adaptations of CPython collections, itertools, heapq, and dataclasses tests."""

from collections import Counter, defaultdict, deque, namedtuple
from dataclasses import dataclass
import heapq
import itertools


queue = deque([2, 3])
queue.appendleft(1)
assert queue.popleft() == 1
assert queue.pop() == 3
assert defaultdict(int)["missing"] == 0
assert Counter("banana")["a"] == 3
Point = namedtuple("Point", "x y")
assert Point(1, 2).y == 2

heap = []
heapq.heappush(heap, 3)
heapq.heappush(heap, 1)
assert heapq.heappop(heap) == 1

assert list(itertools.islice(itertools.count(2, 2), 3)) == [2, 4, 6]
assert list(itertools.permutations("ab")) == [("a", "b"), ("b", "a")]

@dataclass(frozen=True)
class User:
    name: str
    active: bool = True


assert User("PS5").active
assert User("PS5") == User("PS5")

print("test_data_structures: PASS")
