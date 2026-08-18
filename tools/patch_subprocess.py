"""Keep subprocess importable when PS5 cannot provide fork_exec."""

import pathlib
import sys


source_path, output_path = map(pathlib.Path, sys.argv[1:3])
source = source_path.read_text()
old = """if _can_fork_exec:
    from _posixsubprocess import fork_exec as _fork_exec
"""
new = """if _can_fork_exec:
    try:
        from _posixsubprocess import fork_exec as _fork_exec
    except ImportError:
        # PS5 cannot execute ordinary child ELFs from this payload.
        _can_fork_exec = False
"""
if old not in source:
    raise SystemExit("subprocess fork_exec import changed upstream")
output_path.write_text(source.replace(old, new))
