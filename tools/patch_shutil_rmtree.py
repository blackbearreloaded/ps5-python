"""Select the PS5-compatible tempfile cleanup path."""

import pathlib
import sys


source_path, output_path = map(pathlib.Path, sys.argv[1:3])
source = source_path.read_text()
old = """_use_fd_functions = ({os.open, os.stat, os.unlink, os.rmdir} <=
                     os.supports_dir_fd and
                     os.scandir in os.supports_fd and
                     os.stat in os.supports_follow_symlinks)
_rmtree_impl = _rmtree_safe_fd if _use_fd_functions else _rmtree_unsafe
"""
new = """# PS5 advertises fd support but os.scandir(fd) returns ENOTSUP.
# Use the official path-based fallback so TemporaryDirectory cleanup works.
_use_fd_functions = False
_rmtree_impl = _rmtree_unsafe
"""
if old not in source:
    raise SystemExit("shutil rmtree selector changed upstream")
output_path.write_text(source.replace(old, new))
