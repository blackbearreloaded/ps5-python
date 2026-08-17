"""PS5 adaptation of CPython Lib/test/test_os.py."""

import os


assert os.name in ("posix", "nt")
assert os.sep in ("/", "\\")
assert os.path.basename(os.path.join("tmp", "example.txt")) == "example.txt"
assert os.path.dirname(os.path.join("tmp", "example.txt")) == "tmp"
assert os.path.abspath(".")
assert isinstance(os.getcwd(), str)
assert isinstance(os.listdir("."), list)
assert hasattr(os.environ, "keys")
assert os.urandom(0) == b""
random_a = os.urandom(16)
random_b = os.urandom(16)
assert len(random_a) == 16
assert len(random_b) == 16
assert random_a != random_b
if hasattr(os, "getrandom"):
    random_c = os.getrandom(16)
    assert len(random_c) == 16
    assert random_c != random_a

test_base = "/data/python" if os.path.isdir("/data/python") else os.getcwd()
test_root = os.path.join(test_base, ".cpython_ps5_os_test_{0}".format(os.getpid()))
test_file = os.path.join(test_root, "sample.txt")
renamed_file = os.path.join(test_root, "renamed.txt")
os.mkdir(test_root)
try:
    fd = os.open(test_file, os.O_CREAT | os.O_WRONLY, 0o600)
    try:
        assert os.write(fd, b"filesystem works") == 16
    finally:
        os.close(fd)

    assert os.path.exists(test_file)
    assert os.path.isfile(test_file)
    assert os.stat(test_file).st_size == 16
    assert "sample.txt" in os.listdir(test_root)
    os.rename(test_file, renamed_file)
    assert not os.path.exists(test_file)
    assert os.path.exists(renamed_file)
    fd = os.open(renamed_file, os.O_RDONLY)
    try:
        assert os.fstat(fd).st_size == 16
        assert os.lseek(fd, 0, os.SEEK_END) == 16
        assert os.lseek(fd, 0, os.SEEK_SET) == 0
        assert os.read(fd, 32) == b"filesystem works"
    finally:
        os.close(fd)
finally:
    if os.path.exists(test_file):
        os.remove(test_file)
    if os.path.exists(renamed_file):
        os.remove(renamed_file)
    if os.path.exists(test_root):
        os.rmdir(test_root)

print("test_os: PASS")
