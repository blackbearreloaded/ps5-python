"""Prepare an app-local pure-Python dependency bundle on the host.

This runs on the development machine, never on the PS5.  It intentionally
accepts only universal wheels because native extensions must be linked into
the PS5 interpreter separately.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("app", type=Path, help="application directory")
    parser.add_argument(
        "--requirements",
        type=Path,
        help="requirements file (default: <app>/requirements.txt)",
    )
    args = parser.parse_args()

    app = args.app.resolve()
    manifest_path = app / "app.json"
    if not manifest_path.is_file():
        parser.error(f"missing app manifest: {manifest_path}")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    requirements = args.requirements or app / "requirements.txt"
    requirements = requirements.resolve()

    if requirements.is_file():
        requirement_args = ["--requirement", str(requirements)]
    else:
        dependencies = manifest.get("dependencies", [])
        if not isinstance(dependencies, list) or not all(
            isinstance(item, str) and item.strip() for item in dependencies
        ):
            parser.error("manifest dependencies must be a list of strings")
        if not dependencies:
            print(f"No dependencies declared for {app}")
            return 0
        requirement_args = dependencies

    lib = app / "lib"
    lib.mkdir(parents=True, exist_ok=True)
    command = [
        sys.executable,
        "-m",
        "pip",
        "install",
        "--disable-pip-version-check",
        "--no-compile",
        "--upgrade",
        "--only-binary=:all:",
        "--platform",
        "any",
        "--implementation",
        "py",
        "--python-version",
        "3.14",
        "--target",
        str(lib),
        *requirement_args,
    ]
    print("Preparing pure-Python dependencies in", lib)
    subprocess.run(command, check=True)

    native_suffixes = {".so", ".pyd", ".dll", ".dylib"}
    native_files = [
        path
        for path in lib.rglob("*")
        if path.is_file() and path.suffix.lower() in native_suffixes
    ]
    if native_files:
        for path in native_files:
            print(f"Native extension is not deployable on PS5: {path}", file=sys.stderr)
        return 2

    # The PS5 FTP filesystem rejects dotted metadata directories.  Package
    # versions therefore need explicit fallbacks or app-owned metadata.
    for metadata in lib.glob("*.dist-info"):
        shutil.rmtree(metadata)
    print("Package bundle ready:", app)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
