# Releasing

The PS5 SDK is not available on GitHub-hosted runners, so PS5 artifacts are
built either on a maintainer's machine or on a self-hosted GitHub Actions
runner labeled `ps5-sdk`.

## Prepare a release

Run the host checks first, then validate the target console:

```sh
make host-suite
PS5_HOST="<your-PS5-IP>" make ps5-test
PS5_HOST="<your-PS5-IP>" make ps5-web
```

Choose a version tag, for example `v0.1.0`. The release workflow attaches
these assets:

- standalone `python.elf`;
- `python-web.elf` and `python-app-supervisor.elf`;
- a compressed runtime bundle containing the ELF files, `cpython-lib/`, web
  assets, and example apps;
- SHA-256 checksums.

## GitHub Actions release workflow

Configure a self-hosted runner with labels `self-hosted` and `ps5-sdk`. It must
have WSL/bash build dependencies, LLVM/clang/lld with `llvm-config`, `git`,
`make`, `tar`, `sha256sum`, and the GitHub CLI (`gh`). The workflow clones the
pinned CPython source, applies the tracked PS5 configure patch, then clones
[ps5-payload-dev/sdk](https://github.com/ps5-payload-dev/sdk) at the selected
`PS5_SDK_REF`, installs it into the runner's temporary workspace, and exports
`PS5_PAYLOAD_SDK` for the build. The workflow currently pins
`PS5_SDK_REF` to `4eb701204fc3f8d31e84cf8ca272974e2be9c867`; update that pin
intentionally when adopting a reviewed SDK revision.

The runner must be trusted because the workflow has `contents: write`
permission to upload release assets.

Create and publish the GitHub Release for the tag. The `release.yml` workflow
then builds and uploads the artifacts automatically. The workflow can also be
run manually with an existing release tag:

```sh
gh workflow run release.yml -f tag=v0.1.0
```

## Local fallback

When no self-hosted runner is available, build locally and upload from the
same machine:

```sh
make source-fetch
sdk_source=$(mktemp -d)
sdk_dir=$(mktemp -d)
sdk_ref="${PS5_SDK_REF:-4eb701204fc3f8d31e84cf8ca272974e2be9c867}"
git clone --depth 1 \
  https://github.com/ps5-payload-dev/sdk.git "$sdk_source"
git -C "$sdk_source" fetch --depth 1 origin "$sdk_ref"
git -C "$sdk_source" checkout --detach "$sdk_ref"
make -C "$sdk_source" DESTDIR="$sdk_dir" install
export PS5_PAYLOAD_SDK="$sdk_dir"
make host-build
bash tools/build_ps5.sh core
bash tools/build_ps5.sh web
bash tools/package_release.sh v0.1.0
gh release upload v0.1.0 dist/python-ps5-v0.1.0-* --clobber
```

`tools/package_release.sh` never includes the SDK or local CPython checkout.
It packages only the generated ELF files, runtime bundle, web assets, and
example apps.

## Before publishing publicly

- Select a top-level license for project-owned code.
- Review [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) and preserve the
  upstream notices required by each dependency.
- Confirm the release asset checksums and test results in the release notes.
- State the tested firmware/payload environment; an ELF built with this
  project is not a stock-console application.
