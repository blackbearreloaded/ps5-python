"""Small POSIX-safe readline compatibility layer for the PS5 runtime.

The PS5 SDK ships the editline headers but not a linkable editline archive.
This module keeps CPython's readline-facing APIs importable and preserves
history/completion state for ``code``, ``pdb``, and ``rlcompleter``.  Actual
line editing is delegated to the console's ordinary input function.
"""

from __future__ import annotations

import builtins

backend = "none"

_history = []
_history_limit = -1
_completer = None
_completer_delims = " \t\n`~!@#$%^&*()-=+[{]}\\|;:'\",<>/?"
_startup_hook = None
_pre_input_hook = None
_line_buffer = ""
_bindings = {}


def readline(prompt=""):
    global _line_buffer
    if _startup_hook is not None:
        _startup_hook()
    line = builtins.input(prompt)
    _line_buffer = line
    return line


def add_history(line):
    if not isinstance(line, str):
        raise TypeError("line must be str")
    _history.append(line)
    if _history_limit >= 0:
        del _history[:-_history_limit or None]


def clear_history():
    _history.clear()


def get_current_history_length():
    return len(_history)


def get_history_item(index):
    if index < 1 or index > len(_history):
        return None
    return _history[index - 1]


def remove_history_item(index):
    del _history[index]


def replace_history_item(index, line):
    _history[index] = line


def set_history_length(length):
    global _history_limit
    _history_limit = int(length)
    if _history_limit >= 0:
        del _history[:-_history_limit or None]


def get_history_length():
    return _history_limit


def read_history_file(filename=None):
    if filename is None:
        raise TypeError("filename required")
    with open(filename, encoding="utf-8") as stream:
        _history[:] = [line.rstrip("\n") for line in stream]


def write_history_file(filename=None):
    if filename is None:
        raise TypeError("filename required")
    with open(filename, "w", encoding="utf-8") as stream:
        for line in _history:
            stream.write(line + "\n")


def append_history_file(nelements, filename=None):
    if filename is None:
        raise TypeError("filename required")
    with open(filename, "a", encoding="utf-8") as stream:
        for line in _history[-nelements:]:
            stream.write(line + "\n")


def set_completer(function=None):
    global _completer
    old, _completer = _completer, function
    return old


def get_completer():
    return _completer


def set_completer_delims(string):
    global _completer_delims
    old, _completer_delims = _completer_delims, string
    return old


def get_completer_delims():
    return _completer_delims


def set_startup_hook(function=None):
    global _startup_hook
    old, _startup_hook = _startup_hook, function
    return old


def get_startup_hook():
    return _startup_hook


def set_pre_input_hook(function=None):
    global _pre_input_hook
    old, _pre_input_hook = _pre_input_hook, function
    return old


def get_pre_input_hook():
    return _pre_input_hook


def parse_and_bind(line):
    if not isinstance(line, str):
        raise TypeError("line must be str")
    if ":" in line:
        key, command = line.split(":", 1)
        _bindings[key.strip()] = command.strip()


def get_line_buffer():
    return _line_buffer


def insert_text(text):
    global _line_buffer
    _line_buffer += text


def get_begidx():
    return 0


def get_endidx():
    return len(_line_buffer)


def redisplay():
    return None


def read_init_file(filename=None):
    if filename is not None:
        with open(filename, encoding="utf-8"):
            pass

