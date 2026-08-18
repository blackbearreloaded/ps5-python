"""Small dataclass implementation for the PS5 runtime foundation."""


class FrozenInstanceError(AttributeError):
    pass


def dataclass(cls=None, *, frozen=False):
    def decorate(target):
        names = list(getattr(target, "__annotations__", {}))
        defaults = {name: getattr(target, name) for name in names if hasattr(target, name)}

        def init(self, *args, **kwargs):
            if len(args) > len(names):
                raise TypeError("too many positional arguments")
            for index, name in enumerate(names):
                if index < len(args):
                    value = args[index]
                elif name in kwargs:
                    value = kwargs.pop(name)
                elif name in defaults:
                    value = defaults[name]
                else:
                    raise TypeError("missing required argument: " + name)
                object.__setattr__(self, name, value)
            if kwargs:
                raise TypeError("unexpected arguments")

        def repr_method(self):
            values = [name + "=" + repr(getattr(self, name)) for name in names]
            return target.__name__ + "(" + ", ".join(values) + ")"

        def eq(self, other):
            return type(self) is type(other) and all(
                getattr(self, name) == getattr(other, name) for name in names
            )

        target.__init__ = init
        target.__repr__ = repr_method
        target.__eq__ = eq
        if frozen:
            def set_attr(self, name, value):
                raise FrozenInstanceError(name)
            target.__setattr__ = set_attr
            target.__delattr__ = set_attr
        return target

    return decorate if cls is None else decorate(cls)
