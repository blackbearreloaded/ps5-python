#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
source_dir="$root_dir/upstream/cpython"
build_dir="$root_dir/build/host"
jobs="${HOST_JOBS:-2}"

if [ ! -f "$source_dir/Include/Python.h" ]; then
    echo "Missing $source_dir; run make source-fetch first." >&2
    exit 1
fi

mkdir -p "$build_dir"
if [ ! -f "$build_dir/Makefile" ]; then
    cd "$build_dir"
    "$source_dir/configure" \
        --without-ensurepip \
        --with-ensurepip=no \
        --disable-test-modules \
        --disable-shared \
        --with-static-libpython
fi

make -C "$build_dir" -j"$jobs" python.exe libpython3.14.a
