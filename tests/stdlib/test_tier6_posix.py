"""Focused CPython 3.14.7-derived tests for POSIX Tier 6 modules."""

import os


# CPython Lib/test/test_fcntl.py, test_resource.py, and test_tty.py are POSIX
# tests.  Keep the desktop suite runnable on Windows while exercising the same
# native APIs on PS5.
if os.name != "posix":
    print("test_tier6_posix: SKIP (POSIX modules unavailable)")
else:
    import fcntl
    import resource
    import termios
    import tty

    # CPython test_fcntl.py: descriptor flags through fcntl().
    read_fd, write_fd = os.pipe()
    try:
        flags = fcntl.fcntl(read_fd, fcntl.F_GETFD)
        assert isinstance(flags, int)
        fcntl.fcntl(read_fd, fcntl.F_SETFD, flags | fcntl.FD_CLOEXEC)
        assert fcntl.fcntl(read_fd, fcntl.F_GETFD) & fcntl.FD_CLOEXEC
    finally:
        os.close(read_fd)
        os.close(write_fd)

    # CPython test_resource.py: rusage and resource-limit tuple conversion.
    usage = resource.getrusage(resource.RUSAGE_SELF)
    assert usage.ru_utime >= 0.0
    soft, hard = resource.getrlimit(resource.RLIMIT_NOFILE)
    assert soft <= hard or hard == resource.RLIM_INFINITY
    resource.setrlimit(resource.RLIMIT_NOFILE, (soft, hard))

    # CPython test_tty.py: raw/cbreak transformations without requiring a
    # usable terminal device in the PS5 sandbox.
    mode = [
        termios.IGNBRK | termios.BRKINT | termios.IXON,
        termios.OPOST,
        termios.PARENB | termios.CS7,
        termios.ECHO | termios.ICANON | termios.ISIG,
        9600,
        9600,
        [0] * 32,
    ]
    tty.cfmakeraw(mode)
    assert mode[0] & termios.IGNBRK == 0
    assert mode[1] & termios.OPOST == 0
    assert mode[2] & termios.CSIZE == termios.CS8 & termios.CSIZE
    assert mode[2] & termios.CS8 == termios.CS8
    assert mode[3] & (termios.ECHO | termios.ICANON | termios.ISIG) == 0
    assert mode[6][termios.VMIN] == 1
    assert mode[6][termios.VTIME] == 0

    cbreak = [0, termios.OPOST, termios.CS8, termios.ECHO | termios.ICANON,
              9600, 9600, [0] * 32]
    tty.cfmakecbreak(cbreak)
    assert cbreak[1] == termios.OPOST
    assert cbreak[3] & (termios.ECHO | termios.ICANON) == 0
    assert cbreak[6][termios.VMIN] == 1
    assert cbreak[6][termios.VTIME] == 0

    print("test_tier6_posix: PASS")
