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

host_python_target=python.exe
if ! make -C "$build_dir" -n python.exe >/dev/null 2>&1; then
    host_python_target=python
fi

make -C "$build_dir" -j"$jobs" "$host_python_target" libpython3.14.a
if [ "$host_python_target" = python ]; then
    cp "$build_dir/python" "$build_dir/python.exe"
fi
