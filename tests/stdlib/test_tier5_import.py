"""PS5 adaptations of CPython 3.14.7 test_abc and test_importlib."""

import abc
import importlib
import importlib.abc
import importlib.machinery
import importlib.util
import sys


# CPython Lib/test/test_abc.py: abstract methods and virtual subclasses.
class Reader(abc.ABC):
    @abc.abstractmethod
    def read(self):
        raise NotImplementedError


assert Reader.__abstractmethods__ == frozenset({"read"})
try:
    Reader()
except TypeError:
    pass
else:
    raise AssertionError("ABC with an abstract method was instantiated")


class ConcreteReader:
    def read(self):
        return "ok"


Reader.register(ConcreteReader)
assert issubclass(ConcreteReader, Reader)
assert isinstance(ConcreteReader(), Reader)
assert ConcreteReader().read() == "ok"


# CPython Lib/test/test_importlib: public import helpers and finder ABCs.
json_module = importlib.import_module("json")
assert json_module.dumps({"answer": 42}) == '{"answer": 42}'
assert importlib.import_module(".abc", "importlib") is importlib.abc

spec = importlib.util.find_spec("abc")
assert spec is not None
assert spec.name == "abc"
assert spec.loader is not None
assert (
    isinstance(spec.loader, importlib.abc.Loader)
    or (isinstance(spec.loader, type) and issubclass(spec.loader, importlib.abc.Loader))
)
assert importlib.machinery.PathFinder is not None
assert importlib.machinery.BuiltinImporter is not None
assert importlib.machinery.FrozenImporter is not None
assert importlib.util.resolve_name("..abc", "pkg.sub") == "pkg.abc"


class Finder(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path=None, target=None):
        return None


finder = Finder()
assert isinstance(finder, importlib.abc.MetaPathFinder)
assert finder.find_spec("missing_module") is None
assert sys.modules["importlib.abc"] is importlib.abc

print("test_tier5_import: PASS")
