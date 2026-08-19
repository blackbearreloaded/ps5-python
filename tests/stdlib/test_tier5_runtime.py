"""Focused CPython 3.14.7-derived tests for Tier 5 runtime utilities."""

import codecs
import contextlib
import gc
import site
import sysconfig
import types
import weakref


# CPython Lib/test/test_contextlib.py: context-manager protocols and helpers.
assert contextlib.nullcontext("value").__enter__() == "value"
with contextlib.nullcontext(42) as value:
    assert value == 42


@contextlib.contextmanager
def managed_value():
    yield "managed"


with managed_value() as value:
    assert value == "managed"

with contextlib.ExitStack() as stack:
    stack.callback(lambda: None)
    assert stack._exit_callbacks

with contextlib.suppress(KeyError):
    {}["missing"]


# CPython Lib/test/test_gc.py: collection state and explicit collection.
was_enabled = gc.isenabled()
gc.disable()
assert not gc.isenabled()
gc.enable()
assert gc.isenabled()
assert isinstance(gc.collect(), int)
if not was_enabled:
    gc.disable()


# CPython Lib/test/test_site.py: user-site calculation remains queryable even
# though the PS5 launcher intentionally disables automatic site import.
assert isinstance(site.PREFIXES, list)
assert site.getuserbase() is None or isinstance(site.getuserbase(), str)
assert site.getusersitepackages() is None or isinstance(site.getusersitepackages(), str)


# CPython Lib/test/test_sysconfig.py: install schemes and static-build vars.
assert sysconfig.get_platform()
assert "posix_prefix" in sysconfig.get_scheme_names()
paths = sysconfig.get_paths()
assert "stdlib" in paths and "purelib" in paths
assert sysconfig.get_config_var("Py_DEBUG") in (0, 1, None)
assert sysconfig.get_python_version().startswith("3.14")


# CPython Lib/test/test_weakref.py: weak-reference lifecycle and proxy types.
class Target:
    pass


target = Target()
reference = weakref.ref(target)
assert reference() is target
del target
gc.collect()
assert reference() is None


# CPython Lib/test/test_codecs.py: registry lookup and basic transforms.
utf8 = codecs.lookup("utf-8")
assert utf8.name == "utf-8"
assert codecs.encode("PS5", "utf-8") == b"PS5"
assert codecs.decode(b"PS5", "utf-8") == "PS5"


# CPython Lib/test/test_types.py: built-in dynamic type helpers.
namespace = types.SimpleNamespace(answer=42)
assert namespace.answer == 42
mapping = types.MappingProxyType({"answer": 42})
assert mapping["answer"] == 42
template = lambda: None
assert isinstance(types.FunctionType(template.__code__, globals()), types.FunctionType)

print("test_tier5_runtime: PASS")
