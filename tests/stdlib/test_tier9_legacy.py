"""Portable Tier 9 checks adapted from CPython 3.14.7 Lib/test.

The upstream command-line and documentation tests use unittest, subprocesses,
and interactive terminals.  These checks retain their deterministic behavior
without requiring those unavailable test harness services on the PS5.
"""

import io

import cmd
import getopt
import optparse
import pydoc
import shlex
import symtable
import webbrowser


assert shlex.split("alpha 'two words' \"three words\"") == [
    "alpha", "two words", "three words"
]
assert shlex.join(["alpha", "two words", ""] ) == "alpha 'two words' ''"
assert shlex.quote("a'b") == "'a'\"'\"'b'"

options, args = getopt.getopt(
    ["-q", "--output=result.txt", "input.py"], "q", ["output="]
)
assert options == [("-q", ""), ("--output", "result.txt")]
assert args == ["input.py"]

parser = optparse.OptionParser(add_help_option=False)
parser.add_option("-q", "--quiet", action="store_true", dest="quiet")
parser.add_option("-o", "--output", dest="output")
parsed, remaining = parser.parse_args(["--quiet", "-o", "out.txt", "item"])
assert parsed.quiet is True
assert parsed.output == "out.txt"
assert remaining == ["item"]


class Shell(cmd.Cmd):
    prompt = ""

    def __init__(self):
        super().__init__(stdin=io.StringIO(), stdout=io.StringIO())
        self.seen = None

    def do_echo(self, arg):
        self.seen = arg
        return True


shell = Shell()
assert shell.parseline("echo hello") == ("echo", "hello", "echo hello")
assert shell.onecmd("echo hello") is True
assert shell.seen == "hello"

table = symtable.symtable(
    "value = 1\ndef add(extra):\n    return value + extra\n",
    "<tier9>",
    "exec",
)
assert "value" in table.get_identifiers()
function = [child for child in table.get_children() if child.get_name() == "add"][0]
assert function.get_name() == "add"
assert function.lookup("extra").is_parameter()

assert pydoc.locate("builtins.str") is str
rendered = pydoc.render_doc(str)
assert "class str" in rendered


class DummyBrowser(webbrowser.BaseBrowser):
    def open(self, url, new=0, autoraise=True):
        self.request = (url, new, autoraise)
        return True


browser = DummyBrowser("dummy")
webbrowser.register("tier9-dummy", None, instance=browser)
assert webbrowser.get("tier9-dummy") is browser
assert browser.open_new_tab("https://example.com") is True
assert browser.request == ("https://example.com", 2, True)

print("test_tier9_legacy: PASS")
