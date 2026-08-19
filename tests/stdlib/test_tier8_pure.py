"""CPython 3.14.7-derived tests for feasible Tier 8 utility modules.

The checks are adapted from ``Lib/test/test_graphlib.py``,
``test_statistics.py``, ``test_cmath.py``, ``test_ipaddress.py``,
``test_colorsys.py``, ``test_calendar.py``, ``test_zoneinfo`` and
``test_wave.py``.  They deliberately exercise deterministic public APIs so
the same script can run under the host interpreter and the PS5 payload.
"""

import binascii
import calendar
import cmath
import colorsys
import io
import ipaddress
import math
import statistics
import wave
from graphlib import CycleError, TopologicalSorter
from fractions import Fraction


# CPython Lib/test/test_graphlib.py: dependency ordering, completion, and
# cycle detection.
sorter = TopologicalSorter({"build": {"compile"}, "compile": {"parse"}, "parse": set()})
sorter.prepare()
assert sorter.get_ready() == ("parse",)
sorter.done("parse")
assert sorter.get_ready() == ("compile",)
sorter.done("compile")
assert sorter.get_ready() == ("build",)
sorter.done("build")
assert not sorter.is_active()
try:
    TopologicalSorter({"a": {"b"}, "b": {"a"}}).prepare()
except CycleError:
    pass
else:
    raise AssertionError("cyclic graph must raise CycleError")


# CPython Lib/test/test_statistics.py: representative central-tendency and
# spread calculations, including an exact Fraction result.
assert statistics.mean([1, 2, 3, 4]) == 2.5
assert statistics.median([1, 3, 2, 4]) == 2.5
assert statistics.mode([2, 1, 2, 3]) == 2
assert statistics.pvariance([1, 2, 3]) == 2 / 3
assert statistics.variance([1, 2, 3]) == 1
assert statistics.mean([Fraction(1), Fraction(2), Fraction(3)]) == Fraction(2)


# CPython Lib/test/test_cmath.py: complex square roots and polar conversion.
assert cmath.sqrt(-1) == 1j
assert cmath.isclose(cmath.exp(1j * cmath.pi), -1 + 0j, abs_tol=1e-15)
radius, angle = cmath.polar(3 + 4j)
assert math.isclose(radius, 5.0)
assert cmath.isclose(cmath.rect(radius, angle), 3 + 4j, abs_tol=1e-12)


# CPython Lib/test/test_ipaddress.py: parsing, membership, and subnet math.
network = ipaddress.ip_network("192.0.2.0/24")
assert ipaddress.ip_address("192.0.2.42") in network
assert ipaddress.ip_address("192.0.3.1") not in network
assert ipaddress.ip_interface("2001:db8::1/64").network.prefixlen == 64
assert list(ipaddress.summarize_address_range(
    ipaddress.ip_address("192.0.2.0"), ipaddress.ip_address("192.0.2.3")
)) == [ipaddress.ip_network("192.0.2.0/30")]


# CPython Lib/test/test_colorsys.py: RGB/HLS/HSV conversion round trips.
rgb = (0.2, 0.4, 0.8)
assert all(math.isclose(a, b, abs_tol=1e-12) for a, b in zip(rgb, colorsys.hsv_to_rgb(*colorsys.rgb_to_hsv(*rgb))))
assert all(math.isclose(a, b, abs_tol=1e-12) for a, b in zip(rgb, colorsys.hls_to_rgb(*colorsys.rgb_to_hls(*rgb))))


# CPython Lib/test/test_calendar.py: leap-year and month layout helpers.
assert calendar.isleap(2024)
assert not calendar.isleap(2023)
assert calendar.monthrange(2024, 2) == (3, 29)
assert list(calendar.Calendar(firstweekday=0).itermonthdays(2024, 2))[:3] == [0, 0, 0]


# CPython Lib/test/test_wave.py: write and read a minimal PCM stream in memory.
wave_buffer = io.BytesIO()
with wave.open(wave_buffer, "wb") as writer:
    writer.setnchannels(1)
    writer.setsampwidth(2)
    writer.setframerate(8000)
    writer.writeframes(b"\x00\x00\xff\x7f")
wave_buffer.seek(0)
with wave.open(wave_buffer, "rb") as reader:
    assert reader.getparams()[:4] == (1, 2, 8000, 2)
    assert reader.readframes(2) == b"\x00\x00\xff\x7f"


# CPython Lib/test/test_binascii.py: binary/text conversion and CRC.
assert binascii.hexlify(b"PS5") == b"505335"
assert binascii.unhexlify(b"505335") == b"PS5"
assert binascii.crc32(b"123456789") == 0xCBF43926


# CPython Lib/test/test_zoneinfo: the package and its explicit fixed-offset
# constructor are always available.  Named zones depend on a PS5 tzdata
# deployment, so that optional check is reported rather than failing startup.
try:
    from zoneinfo import ZoneInfo, ZoneInfoNotFoundError
except ImportError:
    raise AssertionError("zoneinfo package must be bundled")

try:
    utc = ZoneInfo("UTC")
except ZoneInfoNotFoundError:
    print("test_tier8_pure: named zoneinfo skipped (no PS5 tzdata)")
else:
    assert utc.key == "UTC"

print("test_tier8_pure: PASS")
