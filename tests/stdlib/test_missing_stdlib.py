"""Focused coverage for feasible gaps in the CPython standard library.

These checks cover import and small local behaviors only.  They deliberately
avoid subprocesses, GUI/terminal integrations, network access, and unbounded
filesystem scans.  File-backed cases run on PS5 and are skipped by the
Windows host syntax runner, matching the other filesystem tests.
"""

import configparser
import fileinput
import html.parser
import modulefinder
import netrc
import os
import pickletools
import plistlib
import pyclbr
import sched
import stringprep
import tabnanny
import tempfile
import time
import tomllib
import trace
import zipapp
from html.parser import HTMLParser
from xmlrpc import client as xmlrpc_client


# configparser: parse a small configuration without touching the filesystem.
config = configparser.ConfigParser()
config.read_string("[app]\nname = Python\nport = 9091\n")
assert config.get("app", "name") == "Python"
assert config.getint("app", "port") == 9091


# pickletools: decode a small protocol-0 pickle instruction stream.
operations = list(pickletools.genops(b"I42\n."))
assert [operation.name for operation, _, _ in operations] == ["INT", "STOP"]
assert operations[0][1] == 42


# plistlib: round-trip a small XML property list in memory.
plist = plistlib.dumps({"name": "PS5", "enabled": True}, fmt=plistlib.FMT_XML)
assert plistlib.loads(plist) == {"name": "PS5", "enabled": True}


# sched: execute one due event without sleeping or using a real clock.
events = []
scheduler = sched.scheduler(timefunc=lambda: 0, delayfunc=lambda _: None)
scheduler.enter(0, 1, events.append, argument=("ready",))
scheduler.run()
assert events == ["ready"]


# stringprep: exercise the mapping and character-table helpers.
assert stringprep.map_table_b2("A") == "a"
assert stringprep.in_table_c11(" ")


# tomllib: parse a small TOML document from memory.
toml = tomllib.loads("title = 'PS5'\n[app]\nport = 9091\n")
assert toml == {"title": "PS5", "app": {"port": 9091}}


# trace: run a bounded callable through the public tracing API.
assert trace.Trace(count=1, trace=0).runfunc(lambda: 6 * 7) == 42


# time.strptime: this dependency requires the _strptime implementation.
parsed_time = time.strptime("2024-01-02", "%Y-%m-%d")
assert (parsed_time.tm_year, parsed_time.tm_mon, parsed_time.tm_mday) == (
    2024,
    1,
    2,
)


# html.parser.HTMLParser: this dependency requires html._markupbase.
class TagCollector(HTMLParser):
    def __init__(self):
        HTMLParser.__init__(self)
        self.tags = []

    def handle_starttag(self, tag, attrs):
        self.tags.append(tag)


collector = TagCollector()
collector.feed("<main><p>PS5</p></main>")
assert collector.tags == ["main", "p"]
assert html.parser.HTMLParser is HTMLParser


# xmlrpc: serialize and decode a local method response; no server or network.
xml_response = xmlrpc_client.dumps((42,), methodresponse=True)
xml_values, xml_method = xmlrpc_client.loads(xml_response)
assert xml_values == (42,)
assert xml_method is None


if os.name != "nt":
    with tempfile.TemporaryDirectory(prefix="cpython-ps5-missing-") as directory:
        source_path = os.path.join(directory, "sample.py")
        with open(source_path, "w", encoding="utf-8") as stream:
            stream.write(
                "class Sample:\n"
                "    def method(self):\n"
                "        return 42\n"
                "value = 6 * 7\n"
            )

        # fileinput: read one local file through the public iterator API.
        with fileinput.input(files=(source_path,), encoding="utf-8") as lines:
            assert next(lines).rstrip("\n") == "class Sample:"

        # netrc: parse credentials from a local file without contacting a host.
        netrc_path = os.path.join(directory, "netrc")
        with open(netrc_path, "w", encoding="ascii") as stream:
            stream.write("machine example.com login user password secret\n")
        credentials = netrc.netrc(netrc_path)
        assert credentials.authenticators("example.com") == (
            "user",
            None,
            "secret",
        )

        # modulefinder: discover the module created above without executing it.
        finder = modulefinder.ModuleFinder()
        finder.run_script(source_path)
        assert "__main__" in finder.modules

        # pyclbr: inspect the class and method definitions without importing it.
        classes = pyclbr.readmodule_ex("sample", [directory])
        assert "Sample" in classes
        assert "method" in classes["Sample"].methods

        # tabnanny: validate the clean source file.
        assert tabnanny.check(source_path) is None

        # zipapp: build a small archive; do not execute it.
        archive_path = os.path.join(os.path.dirname(directory), "sample.pyz")
        try:
            zipapp.create_archive(directory, archive_path, main="sample:main")
            assert os.path.isfile(archive_path)
        finally:
            if os.path.exists(archive_path):
                os.unlink(archive_path)
else:
    print("test_missing_stdlib: filesystem-backed cases skipped on host")


print("test_missing_stdlib: PASS")
