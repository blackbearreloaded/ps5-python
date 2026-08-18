"""PS5 adaptations of CPython CSV, decimal, and ElementTree tests."""

import csv
import decimal
import io
import xml.etree.ElementTree as ElementTree


stream = io.StringIO()
csv.writer(stream).writerow(["name", "value,with comma"])
stream.seek(0)
assert next(csv.reader(stream)) == ["name", "value,with comma"]

assert decimal.Decimal("10.25") + decimal.Decimal("0.75") == decimal.Decimal("11.00")

root = ElementTree.fromstring("<root><item id='1'>value</item></root>")
assert root.find("item").text == "value"
assert ElementTree.tostring(root).startswith(b"<root>")

print("test_data_formats: PASS")
