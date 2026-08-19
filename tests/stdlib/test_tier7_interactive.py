"""Tier 7 interactive helpers, adapted from CPython 3.14.7 Lib/test.

The full upstream ``test_code_module.py``, ``test_pdb.py``, and
``test_readline.py`` suites require a controlling terminal and subprocess
support.  These deterministic checks exercise their portable APIs on PS5.
"""

import io

import code
import pdb
import rlcompleter

try:
    import readline
except ImportError:  # Windows hosts have no POSIX readline module.
    readline = None


console = code.InteractiveConsole({})
assert console.push("answer = 6 * 7") is False
assert console.locals["answer"] == 42
assert console.push("if True:") is True
assert console.push("    answer += 1") is True
assert console.push("") is False
assert console.locals["answer"] == 43


completer = rlcompleter.Completer({"alpha": 1, "alphabet": 2})
assert completer.complete("alp", 0) == "alpha"
assert completer.complete("alp", 1) == "alphabet"
assert completer.complete("alp", 2) is None


if readline is not None:
    old_completer = readline.get_completer()
    readline.set_completer(completer.complete)
    assert readline.get_completer() is completer.complete
    readline.set_completer(old_completer)
    readline.clear_history()
    readline.add_history("first")
    readline.add_history("second")
    assert readline.get_current_history_length() == 2
    assert readline.get_history_item(1) == "first"
    assert readline.get_history_item(2) == "second"
    readline.replace_history_item(0, "updated")
    assert readline.get_history_item(1) == "updated"
    readline.remove_history_item(1)
    assert readline.get_current_history_length() == 1
    readline.clear_history()


debugger = pdb.Pdb(stdin=io.StringIO(), stdout=io.StringIO(), readrc=False)
assert debugger.prompt == "(Pdb) "
assert debugger.canonic("./sample.py").endswith("sample.py")

print("test_tier7_interactive: PASS")
