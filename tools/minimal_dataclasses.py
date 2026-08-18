"""Small dataclass implementation for the PS5 runtime foundation."""


class FrozenInstanceError(AttributeError):
    pass


class _MISSING_TYPE:
    pass


MISSING = _MISSING_TYPE()


class Field:
    def __init__(self, default=MISSING, default_factory=MISSING, **kwargs):
        self.default = default
        self.default_factory = default_factory

    @classmethod
    def __class_getitem__(cls, item):
        return cls


def field(*, default=MISSING, default_factory=MISSING, **kwargs):
    if default is not MISSING and default_factory is not MISSING:
        raise ValueError("cannot specify both default and default_factory")
    return Field(default, default_factory, **kwargs)


def dataclass(cls=None, *, frozen=False, kw_only=False, **kwargs):
    def decorate(target):
        names = list(getattr(target, "__annotations__", {}))
        defaults = {
            name: getattr(target, name) for name in names if hasattr(target, name)
        }
        target.__dataclass_fields__ = {
            name: value if isinstance(value, Field) else Field(default=value)
            for name, value in defaults.items()
        }
        for name in names:
            target.__dataclass_fields__.setdefault(name, Field())

        def init(self, *args, **kwargs):
            if kw_only and args:
                raise TypeError("keyword-only arguments expected")
            if len(args) > len(names):
                raise TypeError("too many positional arguments")
            for index, name in enumerate(names):
                if index < len(args):
                    value = args[index]
                elif name in kwargs:
                    value = kwargs.pop(name)
                elif name in defaults:
                    default = defaults[name]
                    if isinstance(default, Field):
                        if default.default_factory is not MISSING:
                            value = default.default_factory()
                        elif default.default is not MISSING:
                            value = default.default
                        else:
                            raise TypeError("missing required argument: " + name)
                    else:
                        value = default
                else:
                    raise TypeError("missing required argument: " + name)
                object.__setattr__(self, name, value)
            if kwargs:
                raise TypeError("unexpected arguments")
            post_init = getattr(self, "__post_init__", None)
            if post_init is not None:
                post_init()

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
