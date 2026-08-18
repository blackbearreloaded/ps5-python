"""PS5 adaptations of CPython Tier 1 sys, typing, and datetime tests."""

import sys
import typing
from datetime import date, datetime, timedelta, timezone


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


box: Box[int] = Box(42)
assert box.value == 42
assert isinstance(SupportsValue, type)
assert typing.get_origin(list[int]) is list
assert typing.get_args(dict[str, int]) == (str, int)
assert typing.cast(str, "typed") == "typed"

day = date(2024, 2, 29)
assert day.isoformat() == "2024-02-29"
moment = datetime(2024, 2, 29, 12, 30, tzinfo=timezone.utc)
assert moment.date() == day
assert moment.astimezone(timezone(timedelta(hours=2))).hour == 14
assert (moment + timedelta(days=1)).day == 1
assert timezone.utc.utcoffset(moment) == timedelta(0)

print("test_tier1: PASS")
