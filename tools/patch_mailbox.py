"""Use Maildir's atomic rename fallback when PS5 lacks hard-link support."""

import pathlib
import sys


source_path, output_path = map(pathlib.Path, sys.argv[1:3])
source = source_path.read_text()
old = """            try:
                os.link(tmp_file.name, dest)
            except (AttributeError, PermissionError):
                os.rename(tmp_file.name, dest)
            else:
                os.remove(tmp_file.name)
"""
new = """            try:
                os.link(tmp_file.name, dest)
            except (AttributeError, PermissionError, OSError):
                # PS5 filesystems may report hard-link support as OSError
                # (with no useful errno). Preserve clash behavior and use the
                # upstream rename fallback when the destination is absent.
                if os.path.exists(dest):
                    raise
                os.rename(tmp_file.name, dest)
            else:
                os.remove(tmp_file.name)
"""
if old not in source:
    raise SystemExit("mailbox Maildir.add link selector changed upstream")
output_path.write_text(source.replace(old, new))
