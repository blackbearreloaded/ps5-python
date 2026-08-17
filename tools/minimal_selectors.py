"""Small select-based selectors implementation for the PS5 runtime."""

import select


EVENT_READ = 1
EVENT_WRITE = 2


class SelectorKey:
    __slots__ = ("fileobj", "fd", "events", "data")

    def __init__(self, fileobj, fd, events, data):
        self.fileobj = fileobj
        self.fd = fd
        self.events = events
        self.data = data


class DefaultSelector:
    def __init__(self):
        self._keys = {}

    def register(self, fileobj, events, data=None):
        fd = fileobj.fileno()
        if not events & (EVENT_READ | EVENT_WRITE):
            raise ValueError("Invalid events")
        if fd in self._keys:
            raise KeyError(fd)
        key = SelectorKey(fileobj, fd, events, data)
        self._keys[fd] = key
        return key

    def unregister(self, fileobj):
        fd = fileobj.fileno()
        return self._keys.pop(fd)

    def modify(self, fileobj, events, data=None):
        self.unregister(fileobj)
        return self.register(fileobj, events, data)

    def select(self, timeout=None):
        keys = list(self._keys.values())
        readable = [key.fileobj for key in keys if key.events & EVENT_READ]
        writable = [key.fileobj for key in keys if key.events & EVENT_WRITE]
        ready_read, ready_write, _ = select.select(readable, writable, [], timeout)
        result = []
        for fileobj in ready_read + ready_write:
            key = self._keys.get(fileobj.fileno())
            if key is None:
                continue
            mask = 0
            if fileobj in ready_read:
                mask |= EVENT_READ
            if fileobj in ready_write:
                mask |= EVENT_WRITE
            result.append((key, mask))
        return result

    def get_map(self):
        return self._keys

    def close(self):
        self._keys.clear()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()


DefaultSelector = DefaultSelector

