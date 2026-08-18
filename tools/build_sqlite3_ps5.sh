#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk_dir="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"
version="${SQLITE_VERSION:-3460100}"
year="${SQLITE_YEAR:-2024}"
source_dir="$root_dir/build/ps5/deps/src/sqlite-autoconf-$version"
archive="$root_dir/build/ps5/deps/src/sqlite-autoconf-$version.tar.gz"
prefix="$root_dir/build/ps5/deps/sqlite3"
url="https://www.sqlite.org/$year/sqlite-autoconf-$version.tar.gz"
sha256="67d3fe6d268e6eaddcae3727fce58fcc8e9c53869bdd07a0c61e38ddf2965071"
jobs="${PS5_JOBS:-$(nproc 2>/dev/null || echo 2)}"

if [ ! -f "$sdk_dir/toolchain/prospero.sh" ]; then
    echo "Missing PS5 SDK: $sdk_dir" >&2
    exit 1
fi

source "$sdk_dir/toolchain/prospero.sh"
mkdir -p "$(dirname "$archive")" "$prefix/include" "$prefix/lib"

if [ ! -f "$archive" ]; then
    curl --fail --location --silent --show-error --output "$archive" "$url"
fi
printf '%s  %s\n' "$sha256" "$archive" | sha256sum --check --status

if [ ! -f "$source_dir/sqlite3.c" ]; then
    tar -xzf "$archive" -C "$(dirname "$source_dir")"
fi

if [ ! -f "$prefix/lib/libsqlite3.a" ] || [ "$source_dir/sqlite3.c" -nt "$prefix/lib/libsqlite3.a" ]; then
    compiler="${CC:-$sdk_dir/bin/prospero-clang}"
    "$compiler" -O2 -fPIC \
        -DSQLITE_THREADSAFE=1 \
        -DSQLITE_OMIT_LOAD_EXTENSION=1 \
        -DSQLITE_DEFAULT_MEMSTATUS=0 \
        -I"$source_dir" -c "$source_dir/sqlite3.c" -o "$source_dir/sqlite3.o"
    ar rcs "$prefix/lib/libsqlite3.a" "$source_dir/sqlite3.o"
    cp "$source_dir/sqlite3.h" "$prefix/include/sqlite3.h"
    cp "$source_dir/sqlite3ext.h" "$prefix/include/sqlite3ext.h"
fi

echo "SQLite 3.46.1: $prefix"
