"""CPython 3.14.7-derived tests for compilation and doctest utilities.

The cases are adapted from ``Lib/test/test_doctest.py``, ``test_py_compile.py``,
``test_compileall.py``, ``test_codeop.py``, and ``test_code.py``.  They stay
small and import-light so the same script runs in the PS5 payload.
"""

import code
import codeop
import compileall
import doctest
import os
import py_compile
import tempfile


# CPython Lib/test/test_codeop.py: incomplete input is distinguishable from
# complete input and syntax errors.
assert codeop.compile_command("if True:") is None
compiled = codeop.compile_command("answer = 6 * 7\n")
namespace = {}
exec(compiled, namespace)
assert namespace["answer"] == 42
try:
    codeop.compile_command("answer =")
except SyntaxError:
    pass
else:
    raise AssertionError("invalid code must raise SyntaxError")


# CPython Lib/test/test_code.py: the interpreter executes complete commands
# and reports incomplete commands for a caller to continue buffering.
interpreter_namespace = {}
interpreter = code.InteractiveInterpreter(interpreter_namespace)
assert interpreter.runsource("answer = 6 * 7") is False
assert interpreter_namespace["answer"] == 42
assert interpreter.runsource("if True:") is True


# CPython Lib/test/test_doctest.py: parse and run an example through the
# public DocTestRunner API without relying on module source recovery.
parser = doctest.DocTestParser()
example = parser.get_doctest(">>> 6 * 7\n42\n", {}, "tier7", "tier7.py", 0)
runner = doctest.DocTestRunner()
runner.run(example)
summary = runner.summarize(verbose=False)
assert summary.failed == 0
assert summary.attempted == 1


# CPython Lib/test/test_py_compile.py and test_compileall.py: source files
# compile to importable PEP 3147 bytecode, both directly and recursively.  The
# desktop harness runs under a restricted Windows sandbox, so leave the
# filesystem portion for the POSIX PS5 run (as the other filesystem tests do).
if os.name != "nt":
    with tempfile.TemporaryDirectory(prefix="cpython-ps5-tier7-") as directory:
        source = os.path.join(directory, "sample.py")
        nested = os.path.join(directory, "nested")
        os.mkdir(nested)
        nested_source = os.path.join(nested, "module.py")
        with open(source, "w", encoding="utf-8") as stream:
            stream.write("value = 6 * 7\n")
        with open(nested_source, "w", encoding="utf-8") as stream:
            stream.write("nested_value = 7 * 6\n")

        pyc = py_compile.compile(source, doraise=True)
        assert isinstance(pyc, str)
        assert os.path.isfile(pyc)
        assert compileall.compile_file(source, quiet=2, force=True)
        assert compileall.compile_dir(directory, quiet=2, force=True)
        assert os.path.isfile(nested_source.replace(".py", ".pyc")) or os.path.isdir(
            os.path.join(nested, "__pycache__")
        )
else:
    print("test_tier7_compile: filesystem compilation skipped on host")


print("test_tier7_compile: PASS")
