"""PS5 adaptations of CPython 3.14.7 inspection and code tests."""

import ast
import dis
import inspect
import io
import sys


# CPython Lib/test/test_ast.py: parsing, tree walking, source locations,
# literal evaluation, and source regeneration.
tree = ast.parse("answer = 6 * 7\n")
assert isinstance(tree, ast.Module)
assign = tree.body[0]
assert isinstance(assign, ast.Assign)
assert assign.targets[0].id == "answer"
assert ast.literal_eval(ast.parse("{'answer': 42}", mode="eval").body) == {
    "answer": 42
}
assert ast.dump(tree, include_attributes=False).startswith("Module(")
assert "answer = 6 * 7" in ast.unparse(tree)
assert list(ast.iter_fields(assign))
assert list(ast.iter_child_nodes(tree))


class Rename(ast.NodeTransformer):
    def visit_Name(self, node):
        node.id = "result"
        return node


renamed = Rename().visit(tree)
ast.fix_missing_locations(renamed)
assert "result = 6 * 7" in ast.unparse(renamed)


# CPython Lib/test/test_dis.py: instruction iteration, metadata, and output.
def add_one(value):
    return value + 1


instructions = list(dis.get_instructions(add_one))
assert instructions
assert any(item.opname in ("RETURN_VALUE", "RETURN_CONST") for item in instructions)
assert dis.Bytecode(add_one).codeobj is add_one.__code__
assert "add_one" in dis.code_info(add_one)
output = io.StringIO()
dis.dis(add_one, file=output)
assert "RETURN" in output.getvalue()


# CPython Lib/test/test_inspect.py: object predicates, signatures, source,
# and argument binding.
assert inspect.isfunction(add_one)
assert inspect.iscode(add_one.__code__)
assert inspect.signature(add_one).parameters["value"].kind is inspect.Parameter.POSITIONAL_OR_KEYWORD
assert inspect.signature(add_one).bind(4).arguments == {"value": 4}
assert inspect.getfullargspec(add_one).args == ["value"]
assert inspect.getdoc(add_one) is None
# The PS5 aggregate launcher executes this file in a namespace that is not
# registered as the `__main__` module, so module member recovery is optional.
module = inspect.getmodule(add_one)
assert module is None or hasattr(module, "__name__")
if not sys.platform.startswith("freebsd"):
    # The desktop test runs from a real file.  The PS5 launcher executes the
    # uploaded script from an in-memory command string, so source recovery is
    # unavailable there even though code metadata and frame inspection work.
    assert "def add_one" in inspect.getsource(add_one)

frame = inspect.currentframe()
assert frame is not None
assert inspect.isframe(frame)
frame_filename = inspect.getframeinfo(frame).filename
if not sys.platform.startswith("freebsd"):
    assert frame_filename.endswith("test_dis.py")

print("test_dis: PASS")
