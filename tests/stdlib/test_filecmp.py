"""Focused CPython 3.14.7-derived tests for Tier 6 text/file utilities.

The assertions are adapted from Lib/test/test_string/, test_textwrap.py,
test_difflib.py, test_mimetypes.py, test_uuid.py, test_stat.py, and
test_filecmp.py in the pinned upstream CPython tree.
"""

import filecmp
import mimetypes
import os
import stat
import tempfile
import uuid
from string import Formatter, Template

import difflib
import textwrap


# CPython Lib/test/test_string/: Template and Formatter basics.
assert Template("Hello, $name!").substitute(name="PS5") == "Hello, PS5!"
assert Template("$$ $name").safe_substitute() == "$ $name"
assert Formatter().format("{0}:{answer}", "PS5", answer=42) == "PS5:42"


# CPython Lib/test/test_textwrap.py: wrapping, dedenting, and indentation.
wrapped = textwrap.wrap("one two three four", width=7)
assert wrapped == ["one two", "three", "four"]
assert textwrap.dedent("  alpha\n  beta\n") == "alpha\nbeta\n"
assert textwrap.indent("alpha\nbeta", "> ") == "> alpha\n> beta"


# CPython Lib/test/test_difflib.py: opcodes and unified textual deltas.
matcher = difflib.SequenceMatcher(None, "abc", "abXc")
assert ("insert", 2, 2, 2, 3) in matcher.get_opcodes()
assert list(difflib.unified_diff(["old\n"], ["new\n"], lineterm=""))[3:] == [
    "-old\n",
    "+new\n",
]


# CPython Lib/test/test_mimetypes.py: built-in type guessing and extensions.
assert mimetypes.guess_type("index.html")[0] == "text/html"
assert mimetypes.guess_type("unknown.ps5")[0] is None
mime_db = mimetypes.MimeTypes()
mime_db.add_type("application/x-ps5", ".ps5")
assert mime_db.guess_type("demo.ps5")[0] == "application/x-ps5"


# CPython Lib/test/test_uuid.py: RFC 4122 parsing, conversion, and generation.
parsed = uuid.UUID("12345678-1234-5678-1234-567812345678")
assert parsed.hex == "12345678123456781234567812345678"
assert parsed.bytes == bytes.fromhex(parsed.hex)
assert parsed.urn == "urn:uuid:12345678-1234-5678-1234-567812345678"
generated = uuid.uuid4()
assert generated.version == 4
assert generated.variant == uuid.RFC_4122
assert uuid.uuid5(uuid.NAMESPACE_DNS, "python.org").version == 5


# CPython Lib/test/test_stat.py: POSIX mode predicates and formatting.
assert stat.S_ISREG(stat.S_IFREG)
assert stat.S_ISDIR(stat.S_IFDIR)
assert stat.filemode(stat.S_IFREG | 0o644) == "-rw-r--r--"


# CPython Lib/test/test_filecmp.py: shallow/deep file comparisons and cache.
# The Windows host runner intentionally does not exercise its restricted temp
# directory; the same block runs on PS5's /user/temp during the core suite.
if os.name != "nt":
    with tempfile.TemporaryDirectory() as directory:
        first = os.path.join(directory, "first.txt")
        same = os.path.join(directory, "same.txt")
        different = os.path.join(directory, "different.txt")
        with open(first, "w", encoding="utf-8") as handle:
            handle.write("same contents\n")
        with open(same, "w", encoding="utf-8") as handle:
            handle.write("same contents\n")
        with open(different, "w", encoding="utf-8") as handle:
            handle.write("different contents\n")
        assert filecmp.cmp(first, same, shallow=False)
        assert not filecmp.cmp(first, different, shallow=False)
        filecmp.clear_cache()
        assert not filecmp._cache

print("test_filecmp: PASS")
