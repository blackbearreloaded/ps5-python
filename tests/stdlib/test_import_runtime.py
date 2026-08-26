# PS5-Python - CPython for the PlayStation 5.
# Copyright (C) 2026 BlackBearReloaded
# SPDX-License-Identifier: GPL-3.0-or-later

"""PS5 adaptations of CPython importlib, pathlib, and zipimport tests."""

import pathlib
import zipimport
import _stat
import stat
import posixpath


path = pathlib.PurePosixPath("/data/python") / "assets" / "index.html"
assert str(path) == "/data/python/assets/index.html"
assert path.name == "index.html"
assert path.parent == pathlib.PurePosixPath("/data/python/assets")
assert stat.S_ISDIR(stat.S_IFDIR)
assert stat.S_ISREG(stat.S_IFREG)
assert posixpath.join("/data", "python") == "/data/python"
assert hasattr(_stat, "S_IFDIR")
assert zipimport.zipimporter is not None

print("test_import_runtime: PASS")
