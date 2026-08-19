#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk_dir="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"
version="${BZIP2_VERSION:-1.0.8}"
source_dir="$root_dir/build/ps5/deps/src/bzip2-$version"
archive="$root_dir/build/ps5/deps/src/bzip2-$version.tar.gz"
prefix="$root_dir/build/ps5/deps/bzip2"
url="https://sourceware.org/pub/bzip2/bzip2-$version.tar.gz"
sha256="ab5a03176ee106d3f0fa90e381da478ddae405918153cca248e682cd0c4a2269"

if [ ! -f "$sdk_dir/toolchain/prospero.sh" ]; then
    echo "Missing PS5 SDK: $sdk_dir" >&2
    exit 1
fi

mkdir -p "$(dirname "$archive")" "$prefix"
if [ ! -f "$archive" ]; then
    curl --fail --location --silent --show-error --output "$archive" "$url"
fi
printf '%s  %s\n' "$sha256" "$archive" | sha256sum --check --status
if [ ! -f "$source_dir/bzlib.c" ]; then
    tar -xzf "$archive" -C "$(dirname "$source_dir")"
fi

if [ ! -f "$prefix/lib/libbz2.a" ]; then
    cd "$source_dir"
    make clean >/dev/null 2>&1 || true
    make -j"${PS5_JOBS:-$(nproc 2>/dev/null || echo 2)}" libbz2.a \
        CC="$sdk_dir/bin/prospero-clang" \
        AR="$sdk_dir/bin/prospero-ar" \
        RANLIB="$sdk_dir/bin/prospero-ranlib" \
        CFLAGS="-O2 -fPIC"
    mkdir -p "$prefix/include" "$prefix/lib"
    cp bzlib.h "$prefix/include/bzlib.h"
    cp libbz2.a "$prefix/lib/libbz2.a"
fi

echo "bzip2 $version: $prefix"
