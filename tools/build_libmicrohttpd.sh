#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk_dir="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"
lib_version="${LIBMICROHTTPD_VERSION:-1.0.1}"
install_root="$root_dir/build/ps5/deps"

if [ ! -f "$sdk_dir/toolchain/prospero.sh" ]; then
    echo "Missing PS5 SDK: $sdk_dir" >&2
    exit 1
fi

source "$sdk_dir/toolchain/prospero.sh"
prefix_dir="$install_root${PS5_HBROOT:-/user/homebrew}"

if [ -f "$prefix_dir/include/microhttpd.h" ] &&
   [ -f "$prefix_dir/lib/libmicrohttpd.a" ]; then
    echo "libmicrohttpd $lib_version is already installed in $prefix_dir"
    exit 0
fi

temp_dir=$(mktemp -d)
trap 'rm -rf -- "$temp_dir"' EXIT

archive="$temp_dir/libmicrohttpd-$lib_version.tar.gz"
source_dir="$temp_dir/libmicrohttpd-$lib_version"

if command -v curl >/dev/null 2>&1; then
    curl --fail --location --output "$archive" \
        "https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-$lib_version.tar.gz"
else
    wget --no-verbose --output-document="$archive" \
        "https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-$lib_version.tar.gz"
fi
tar xf "$archive" -C "$temp_dir"

cd "$source_dir"
CFLAGS="${LIBMICROHTTPD_CFLAGS:--O1}" \
    ./configure \
        --prefix="$PS5_HBROOT" \
        --host=x86_64 \
        --disable-shared \
        --enable-static \
        --disable-curl \
        --disable-examples
${MAKE} install DESTDIR="$install_root"

if ! grep -q "MHD_ALLOW_UPGRADE" "$prefix_dir/include/microhttpd.h" ||
   ! grep -q "MHD_create_response_for_upgrade" "$prefix_dir/include/microhttpd.h"; then
    echo "libmicrohttpd was built without HTTP upgrade support" >&2
    exit 1
fi

echo "Installed libmicrohttpd $lib_version with HTTP upgrade support"
