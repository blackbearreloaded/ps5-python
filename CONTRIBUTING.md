# Contributing

Thanks for helping improve CPythonPS5. This project targets CPython 3.14.7 in
a jailbroken PS5 payload environment, so changes need both ordinary Python
care and PS5-specific validation when they cross the native boundary.

## Before opening a pull request

1. Keep the working tree focused; do not commit `build/`, `upstream/`, SDK
   files, generated app `lib/` directories, or compiled artifacts.
2. Run the host checks:

   ```sh
   make host-suite
   python tests/stdlib/test_missing_stdlib.py
   ```

3. For C or build changes, also run:

   ```sh
   make format-check
   make tidy
   ```

4. For runtime or standard-library changes, update the relevant status and
   limitation documentation and add a bounded test.
5. For PS5-facing changes, run the applicable `ps5-*` target and report the
   firmware/payload environment and any skipped capabilities.

## Scope rules

- Keep upstream CPython sources pinned to the commit in
  [CPYTHON_VERSION.txt](CPYTHON_VERSION.txt).
- Prefer official CPython pure-Python modules where the PS5 primitives support
  them; do not add compatibility stubs for unavailable GUI, PTY, or kernel
  features.
- Do not commit the PS5 SDK or artifacts derived from SDK files that the SDK
  license does not permit redistributing.
- When adding a third-party dependency, document its source and license in
  [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Pull-request checklist

- [ ] Host tests pass.
- [ ] Formatting and tidy checks pass when native code changed.
- [ ] PS5 validation is included or the limitation is documented.
- [ ] README/status/roadmap documentation is current.
- [ ] No generated artifacts or local SDK/source trees are staged.
