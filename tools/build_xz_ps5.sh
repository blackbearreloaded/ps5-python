#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk_dir="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"
version="${XZ_VERSION:-5.6.3}"
source_dir="$root_dir/build/ps5/deps/src/xz-$version"
archive="$root_dir/build/ps5/deps/src/xz-$version.tar.gz"
prefix="$root_dir/build/ps5/deps/xz"
url="https://github.com/tukaani-project/xz/releases/download/v$version/xz-$version.tar.gz"
sha256="b1d45295d3f71f25a4c9101bd7c8d16cb56348bbef3bbc738da0351e17c73317"

if [ ! -f "$sdk_dir/toolchain/prospero.sh" ]; then
    echo "Missing PS5 SDK: $sdk_dir" >&2
    exit 1
fi

mkdir -p "$(dirname "$archive")" "$prefix"
if [ ! -f "$archive" ]; then
    curl --fail --location --silent --show-error --output "$archive" "$url"
fi
printf '%s  %s\n' "$sha256" "$archive" | sha256sum --check --status
if [ ! -f "$source_dir/configure" ]; then
    tar -xzf "$archive" -C "$(dirname "$source_dir")"
fi

if [ ! -f "$prefix/lib/liblzma.a" ]; then
    cd "$source_dir"
    rm -f Makefile config.status
    ac_cv_func_wcwidth=no ./configure \
        --build=x86_64-pc-linux-gnu \
        --host=x86_64-pc-freebsd \
        --disable-shared --enable-static \
        --disable-doc --disable-scripts \
        --disable-xz --disable-xzdec --disable-lzmadec \
        --disable-lzmainfo --prefix="$prefix" \
        CC="$sdk_dir/bin/prospero-clang" \
        AR="$sdk_dir/bin/prospero-ar" \
        RANLIB="$sdk_dir/bin/prospero-ranlib" \
        CFLAGS="-O2"
    make -j"${PS5_JOBS:-$(nproc 2>/dev/null || echo 2)}"
    mkdir -p "$prefix/include" "$prefix/lib"
    cp src/liblzma/.libs/liblzma.a "$prefix/lib/liblzma.a"
    cp src/liblzma/api/lzma.h "$prefix/include/lzma.h"
    cp -R src/liblzma/api/lzma "$prefix/include/lzma"
fi

echo "xz/liblzma $version: $prefix"
