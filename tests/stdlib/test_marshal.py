"""Portable CPython 3.14.7-derived checks for Tier 9 core modules."""

import __future__
import __main__
import _thread
import builtins
import copyreg
import marshal


for name in __future__.all_feature_names:
    feature = getattr(__future__, name)
    assert isinstance(feature, __future__._Feature)
    assert len(feature.getOptionalRelease()) == 5
    compile("", "<future>", "exec", feature.compiler_flag)

scope = {}
exec("from __future__ import annotations\nvalue: MissingType", scope)
assert scope["__annotations__"]["value"] == "MissingType"

values = [None, True, 42, 1 << 100, 3.5, 2 + 4j, "PS5", b"bytes",
          (1, "two"), [3, 4], {"answer": 42}]
for value in values:
    assert marshal.loads(marshal.dumps(value)) == value
code = compile("answer = 42", "<marshal>", "exec")
assert marshal.loads(marshal.dumps(code)).co_filename == "<marshal>"


class Registered:
    pass


def reduce_registered(value):
    return Registered, ()


copyreg.pickle(Registered, reduce_registered)
assert copyreg.dispatch_table[Registered] is reduce_registered
copyreg.dispatch_table.pop(Registered, None)

lock = _thread.allocate_lock()
assert lock.acquire()
assert not lock.acquire(False)
lock.release()
assert not lock.locked()
assert __main__.__name__ == "__main__"
assert builtins.ValueError is ValueError

print("test_marshal: PASS")
