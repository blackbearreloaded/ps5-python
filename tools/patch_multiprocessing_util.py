"""Patch only the subprocess flag import out of multiprocessing.util for PS5."""

import pathlib
import sys


source_path, output_path = map(pathlib.Path, sys.argv[1:3])
source = source_path.read_text()
old = "from subprocess import _args_from_interpreter_flags  # noqa: F401"
new = """try:
    from subprocess import _args_from_interpreter_flags  # noqa: F401
except ImportError:
    # PS5 cannot launch ordinary child ELFs through libc subprocess APIs.
    def _args_from_interpreter_flags():
        return []
"""
if old not in source:
    raise SystemExit("multiprocessing.util import line changed upstream")
output_path.write_text(source.replace(old, new))
