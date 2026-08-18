#!/usr/bin/env bash
set -eu
root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk_dir="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"
version="${LIBFFI_VERSION:-3.8.0}"
src="$root_dir/build/ps5/deps/src/libffi-$version"
archive="$root_dir/build/ps5/deps/src/libffi-$version.tar.gz"
prefix="$root_dir/build/ps5/deps/libffi"
url="https://github.com/libffi/libffi/releases/download/v$version/libffi-$version.tar.gz"
source "$sdk_dir/toolchain/prospero.sh"
mkdir -p "$(dirname "$archive")"
[ -f "$archive" ] || curl --fail --location --silent --show-error -o "$archive" "$url"
[ -f "$src/configure" ] || tar -xzf "$archive" -C "$(dirname "$src")"
if [ ! -f "$prefix/lib/libffi.a" ]; then
    cd "$src"
    ./configure --disable-builddir --build=x86_64-pc-linux-gnu --host=x86_64-pc-freebsd --disable-shared --enable-static --prefix="$prefix" CFLAGS="-DFFI_NO_EFI64"
    sed -i 's# src/x86/ffiw64\.lo src/x86/win64\.lo##g' Makefile
    sed -i 's/#ifndef __ILP32__/#if !defined(__ILP32__) \&\& !defined(FFI_NO_EFI64)/g' src/x86/ffi64.c
    make -j"${PS5_JOBS:-$(nproc 2>/dev/null || echo 2)}"
    mkdir -p "$prefix/include" "$prefix/lib"
    cp include/*.h "$prefix/include/"
    cp .libs/libffi.a "$prefix/lib/libffi.a"
fi
echo "libffi $version: $prefix"
