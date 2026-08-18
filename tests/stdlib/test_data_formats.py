"""PS5 adaptations of CPython CSV, decimal, and ElementTree tests."""

import csv
import contextlib
import copy
import decimal
import io
import numbers
import weakref
import xml.etree.ElementTree as ElementTree
import xml.etree
import xml
import _weakrefset


stream = io.StringIO()
csv.writer(stream).writerow(["name", "value,with comma"])
stream.seek(0)
assert next(csv.reader(stream)) == ["name", "value,with comma"]

assert decimal.Decimal("10.25") + decimal.Decimal("0.75") == decimal.Decimal("11.00")

root = ElementTree.fromstring("<root><item id='1'>value</item></root>")
assert root.find("item").text == "value"
assert ElementTree.tostring(root).startswith(b"<root>")
assert xml.etree.ElementTree is not None
assert hasattr(ElementTree, "ElementPath") or ElementTree.find is not None
assert isinstance(copy.copy([1]), list)
assert isinstance(decimal.Decimal("1"), numbers.Number)
assert contextlib.nullcontext is not None
assert weakref.ref(root)() is root
assert _weakrefset.WeakSet is not None
assert xml.__name__ == "xml"

print("test_data_formats: PASS")
