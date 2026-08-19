"""Portable Tier 9 legacy-helper checks adapted from CPython 3.14.7.

The source tests are ``test_cmd``, ``test_shlex``, ``test_optparse``,
``test_getopt``, ``test_symtable``, and ``test_pydoc`` in CPython's
``Lib/test``.  Browser, GUI, and terminal integrations are intentionally not
started on the headless PS5 payload.
"""

import cmd
import getopt
import io
import optparse
import shlex


# CPython Lib/test/test_cmd.py: parsing and command dispatch without a TTY.
class Echo(cmd.Cmd):
    def __init__(self):
        super().__init__(stdin=io.StringIO(), stdout=io.StringIO())
        self.seen = []

    def do_echo(self, argument):
        self.seen.append(argument)
        return False


console = Echo()
assert console.parseline("echo hello") == ("echo", "hello", "echo hello")
assert console.onecmd("echo hello") is False
assert console.seen == ["hello"]


# CPython Lib/test/test_shlex.py: POSIX tokenization and quoting helpers.
assert shlex.split('alpha "two words"') == ["alpha", "two words"]
assert shlex.join(["alpha", "two words"]) == "alpha 'two words'"
assert shlex.quote("don't") == "'don'\"'\"'t'"


# CPython Lib/test/test_optparse.py and test_getopt.py.
parser = optparse.OptionParser()
parser.add_option("-n", "--name", dest="name")
options, args = parser.parse_args(["--name", "ps5", "payload"])
assert options.name == "ps5"
assert args == ["payload"]
assert getopt.getopt(["-a", "-b", "value", "tail"], "ab:") == (
    [("-a", ""), ("-b", "value")],
    ["tail"],
)


# CPython Lib/test/test_symtable.py.  _symtable is optional in reduced builds.
try:
    import symtable
except ImportError:
    symtable = None
if symtable is not None:
    table = symtable.symtable(
        "answer = 42\ndef f(): return answer", "<tier9>", "exec"
    )
    assert table.lookup("answer").is_global()
    assert table.lookup("f").is_namespace()


# CPython Lib/test/test_pydoc.py.  Rendering is pure text; server/browser
# startup is excluded because the payload has no desktop.
try:
    import pydoc
except ImportError:
    pydoc = None
if pydoc is not None:
    documentation = pydoc.render_doc(str, renderer=pydoc.plaintext)
    assert "class str" in documentation


print("test_tier9_legacy: PASS")
