# PS5-Python - CPython for the PlayStation 5.
# Copyright (C) 2026 BlackBearReloaded
# SPDX-License-Identifier: GPL-3.0-or-later

"""Verify license notices on tracked first-party code and app manifests."""

import json
import pathlib
import subprocess


ROOT = pathlib.Path(__file__).resolve().parent.parent
CODE_SUFFIXES = {
    ".c",
    ".css",
    ".h",
    ".html",
    ".js",
    ".local",
    ".ps1",
    ".py",
    ".sh",
    ".site",
    ".yaml",
    ".yml",
}
CODE_NAMES = {".clang-format", ".clang-tidy", "Makefile"}
IGNORED_PREFIXES = ("third_party/", "web/vendor/")
COPYRIGHT = "Copyright (C) 2026 BlackBearReloaded"
SPDX = "SPDX-License-Identifier: GPL-3.0-or-later"


def tracked_files():
    output = subprocess.check_output(
        ["git", "ls-files", "-z", "--cached", "--others", "--exclude-standard"],
        cwd=ROOT,
        text=True,
        encoding="utf-8",
    )
    return (pathlib.Path(name) for name in output.split("\0") if name)


def main():
    missing = []
    checked = 0
    for relative in tracked_files():
        normalized = relative.as_posix()
        if normalized.startswith(IGNORED_PREFIXES):
            continue

        path = ROOT / relative
        if normalized.startswith("apps/") and normalized.endswith("/app.json"):
            checked += 1
            manifest = json.loads(path.read_text(encoding="utf-8"))
            if manifest.get("copyright") != COPYRIGHT or manifest.get("license") != "GPL-3.0-or-later":
                missing.append(normalized)
            continue

        if relative.suffix.lower() not in CODE_SUFFIXES and relative.name not in CODE_NAMES:
            continue

        checked += 1
        source = path.read_text(encoding="utf-8")
        if COPYRIGHT not in source or SPDX not in source:
            missing.append(normalized)

    if missing:
        raise SystemExit("Missing PS5-Python license notice:\n" + "\n".join(missing))
    print(f"License notices verified in {checked} first-party files.")


if __name__ == "__main__":
    main()
