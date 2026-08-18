"""PS5 adaptations of CPython pathlib and tempfile tests."""

import os
import pathlib
import tempfile


# CPython Lib/test/test_pathlib.py and Lib/test/test_tempfile.py.
path = pathlib.PurePosixPath("/data/python") / "tmp" / "value.txt"
assert str(path) == "/data/python/tmp/value.txt"
assert path.parent == pathlib.PurePosixPath("/data/python/tmp")

if os.name == "nt":
    # The managed Windows host denies the temporary directory; PS5 exercises
    # the real tempfile APIs below.
    print("test_filesystem: tempfile checks skipped on host")
else:
    temp_root = "/data/python"
    with tempfile.TemporaryDirectory(prefix="cpython-ps5-", dir=temp_root) as directory:
        root = pathlib.Path(directory)
        nested = root / "nested"
        nested.mkdir()
        value = nested / "value.txt"
        value.write_text("PS5 pathlib\n", encoding="utf-8")

        assert value.exists()
        assert value.is_file()
        assert nested.is_dir()
        assert value.read_text(encoding="utf-8") == "PS5 pathlib\n"
        assert value.parent == nested
        assert value.name == "value.txt"
        assert sorted(path.name for path in nested.iterdir()) == ["value.txt"]

    assert not root.exists()

    with tempfile.NamedTemporaryFile(
        mode="w+", encoding="utf-8", dir=temp_root
    ) as handle:
        handle.write("temporary file")
        handle.flush()
        handle.seek(0)
        assert handle.read() == "temporary file"
        named_path = handle.name
    assert not os.path.exists(named_path)

    fd, path = tempfile.mkstemp(prefix="cpython-ps5-", dir=temp_root)
    try:
        os.write(fd, b"mkstemp")
        os.close(fd)
        fd = -1
        with open(path, "rb") as handle:
            assert handle.read() == b"mkstemp"
    finally:
        if fd >= 0:
            os.close(fd)
        os.unlink(path)

print("test_filesystem: PASS")
