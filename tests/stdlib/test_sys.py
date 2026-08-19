"""PS5 adaptations of CPython Tier 1 sys, typing, and datetime tests."""

import sys
import typing
from datetime import date, datetime, timedelta, timezone
from functools import lru_cache, partial


# CPython Lib/test/test_sys.py, Lib/test/test_typing.py, and
# Lib/test/datetimetester.py.
assert isinstance(sys.version, str)
assert sys.version_info[:2] == (3, 14)
assert sys.stdin is not None
assert sys.stdout is not None
assert isinstance(sys.argv, list)

T = typing.TypeVar("T")


class Box(typing.Generic[T]):
    def __init__(self, value: T):
        self.value = value


class SupportsValue(typing.Protocol):
    value: str


class Named(typing.TypedDict):
    name: str
    active: bool


box: Box[int] = Box(42)
assert box.value == 42
assert isinstance(SupportsValue, type)
assert typing.is_typeddict(Named)
assert typing.get_type_hints(Named) == {"name": str, "active": bool}
assert typing.get_origin(list[int]) is list
assert typing.get_args(dict[str, int]) == (str, int)
assert typing.cast(str, "typed") == "typed"


@lru_cache(maxsize=2)
def square(value):
    return value * value


add_two = partial(lambda left, right: left + right, 2)
assert square(4) == 16
assert square.cache_info().hits == 0
assert add_two(3) == 5

day = date(2024, 2, 29)
assert day.isoformat() == "2024-02-29"
moment = datetime(2024, 2, 29, 12, 30, tzinfo=timezone.utc)
assert moment.date() == day
assert moment.astimezone(timezone(timedelta(hours=2))).hour == 14
assert (moment + timedelta(days=1)).day == 1
assert timezone.utc.utcoffset(moment) == timedelta(0)

print("test_sys: PASS")
