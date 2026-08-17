class Parent:
    shared = "parent"

    def value(self):
        return self.shared


class Child(Parent):
    own = "child"


item = Child()
assert item.shared == "parent"
assert item.own == "child"
assert item.value() == "parent"

item.shared = "instance"
assert item.shared == "instance"
assert item.value() == "instance"
assert Parent.shared == "parent"

del item.shared
assert item.shared == "parent"

Child.shared = "class override"
assert item.shared == "class override"
assert item.value() == "class override"

item.dynamic = 99
assert item.dynamic == 99
assert hasattr(item, "dynamic")
del item.dynamic
assert not hasattr(item, "dynamic")

print("attribute_lookup: PASS")
