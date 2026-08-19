"""Keep official sysconfig useful when cross-build data is not installed."""

import pathlib
import sys


path = pathlib.Path(sys.argv[1])
source = path.read_text()
old = """def _init_posix(vars):
    \"\"\"Initialize the module as appropriate for POSIX systems.\"\"\"
    # GH-126920: Make sure we don't overwrite any of the keys already set
    vars.update(_get_sysconfigdata() | vars)
"""
new = """def _init_posix(vars):
    \"\"\"Initialize POSIX variables, including PS5's static-build fallback.\"\"\"
    # The normal installed interpreter provides _sysconfigdata.  This PS5
    # payload intentionally ships only the static runtime, so use the native
    # _sysconfig helper when that generated file is unavailable.
    try:
        build_vars = _get_sysconfigdata()
    except (ImportError, ModuleNotFoundError, AttributeError):
        try:
            import _sysconfig
            build_vars = _sysconfig.config_vars()
        except (ImportError, AttributeError):
            build_vars = {}
    # Keep the standard precedence: caller-provided values win.
    vars.update(build_vars | vars)
"""
if old not in source:
    raise SystemExit("sysconfig POSIX initializer changed upstream")
path.write_text(source.replace(old, new))
