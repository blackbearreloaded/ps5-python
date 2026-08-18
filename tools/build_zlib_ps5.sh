#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk_dir="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"
version="${ZLIB_VERSION:-1.3.1}"
source_dir="$root_dir/build/ps5/deps/src/zlib-$version"
archive="$root_dir/build/ps5/deps/src/zlib-$version.tar.gz"
prefix="$root_dir/build/ps5/deps/zlib"
url="https://zlib.net/fossils/zlib-$version.tar.gz"
sha256="9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23"

if [ ! -f "$sdk_dir/toolchain/prospero.sh" ]; then
    echo "Missing PS5 SDK: $sdk_dir" >&2
    exit 1
fi

source "$sdk_dir/toolchain/prospero.sh"
mkdir -p "$(dirname "$archive")" "$prefix"

if [ ! -f "$archive" ]; then
    curl --fail --location --silent --show-error --output "$archive" "$url"
fi
printf '%s  %s\n' "$sha256" "$archive" | sha256sum --check --status
if [ ! -f "$source_dir/configure" ]; then
    tar -xzf "$archive" -C "$(dirname "$source_dir")"
fi

if [ ! -f "$prefix/lib/libz.a" ]; then
    cd "$source_dir"
    make distclean >/dev/null 2>&1 || true
    CC="$CC" AR="$AR" RANLIB="$RANLIB" ./configure --static --prefix="$prefix"
    make -j"${PS5_JOBS:-$(nproc 2>/dev/null || echo 2)}"
    # The SDK toolchain rewrites configure's install prefix as a target-root
    # path.  Copy the static artefact explicitly into the workspace prefix so
    # the CPython configure/link steps can consume it.
    mkdir -p "$prefix/include" "$prefix/lib"
    cp zlib.h zconf.h "$prefix/include/"
    cp libz.a "$prefix/lib/libz.a"
fi

echo "zlib $version: $prefix"
