#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk_dir="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"
version="${OPENSSL_VERSION:-3.5.2}"
source_dir="$root_dir/build/ps5/deps/src/openssl-$version"
archive="$root_dir/build/ps5/deps/src/openssl-$version.tar.gz"
prefix="$root_dir/build/ps5/deps/openssl"
url="https://github.com/openssl/openssl/releases/download/openssl-$version/openssl-$version.tar.gz"
sha256="c53a47e5e441c930c3928cf7bf6fb00e5d129b630e0aa873b08258656e7345ec"

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

if [ ! -f "$source_dir/Configure" ]; then
    tar -xzf "$archive" -C "$(dirname "$source_dir")"
fi

if [ ! -f "$prefix/lib/libssl.a" ] || [ ! -f "$prefix/lib/libcrypto.a" ]; then
    cd "$source_dir"
    ./Configure BSD-x86_64 no-tests no-apps no-shared no-module \
        --prefix="$prefix"
    make -j"${PS5_JOBS:-$(nproc 2>/dev/null || echo 2)}" build_sw
    make install_sw
fi

echo "OpenSSL $version: $prefix"
