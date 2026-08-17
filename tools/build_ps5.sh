#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
source_dir="$root_dir/upstream/cpython"
build_dir="$root_dir/build/ps5"
build_python="$root_dir/build/host/python.exe"
sdk_dir="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"
hb_dir="$root_dir/build/ps5/deps/user/homebrew"
jobs="${PS5_JOBS:-2}"
launcher="$build_dir/python.elf"
web_launcher="$build_dir/python-web.elf"
runtime_dir="$build_dir/cpython-lib"

if [ ! -f "$source_dir/Include/Python.h" ]; then
    echo "Missing $source_dir; run make source-fetch first." >&2
    exit 1
fi

if [ ! -x "$build_python" ]; then
    echo "Missing $build_python; run the host build first." >&2
    exit 1
fi

if [ ! -f "$sdk_dir/toolchain/prospero.sh" ]; then
    echo "Missing PS5 SDK: $sdk_dir" >&2
    exit 1
fi

source "$sdk_dir/toolchain/prospero.sh"

configure_ps5() {
    mkdir -p "$build_dir"
    cd "$build_dir"
    CONFIG_SITE="$root_dir/tools/ps5.config.site" \
    "$source_dir/configure" \
        --build=x86_64-pc-linux-gnu \
        --host=x86_64-pc-freebsd \
        --with-build-python="$build_python" \
        --disable-test-modules \
        --without-ensurepip \
        --with-ensurepip=no \
        --disable-shared \
        --with-static-libpython \
        --disable-ipv6 \
        --with-libm=no
    cp "$root_dir/tools/ps5-setup.local" "$build_dir/Modules/Setup.local"
}

build_runtime_bundle() {
    mkdir -p "$runtime_dir/encodings"
    cp "$root_dir/tools/minimal_encodings_init.py" \
        "$runtime_dir/encodings/__init__.py"
    cp "$source_dir/Lib/codecs.py" "$runtime_dir/codecs.py"
    cp "$source_dir/Lib/encodings/ascii.py" \
        "$runtime_dir/encodings/ascii.py"
    cp "$source_dir/Lib/encodings/utf_8.py" \
        "$runtime_dir/encodings/utf_8.py"
    cp "$root_dir/tools/minimal_idna.py" "$runtime_dir/encodings/idna.py"

    # os is the Python-level POSIX wrapper; its native posix/time/stat pieces
    # are already compiled into Modules/config.c.
    cp "$root_dir/tools/minimal_selectors.py" "$runtime_dir/selectors.py"
    for module in os.py stat.py genericpath.py posixpath.py abc.py _collections_abc.py io.py socket.py enum.py types.py; do
        cp "$source_dir/Lib/$module" "$runtime_dir/$module"
    done
}

build_launcher() {
    "$sdk_dir/bin/prospero-clang" \
        -DCPYTHON_PS5 \
        -I"$root_dir/include" \
        -I"$build_dir" \
        -I"$source_dir/Include" \
        -I"$source_dir" \
        -I"$source_dir/Include/internal" \
        -o "$launcher" \
        "$root_dir/src/cpython_runner.c" \
        "$root_dir/src/cpython_runtime.c" \
        "$root_dir/src/ps5_time.c" \
        "$root_dir/platform/cpython_ps5_host.c" \
        "$build_dir/Modules/config.o" \
        "$build_dir/libpython3.14.a" \
        -Wl,--wrap=clock_nanosleep -ldl -lpthread
}

build_web_launcher() {
    bash "$root_dir/tools/build_libmicrohttpd.sh"
    "$sdk_dir/bin/prospero-clang" \
        -DCPYTHON_PS5 \
        -I"$root_dir/include" \
        -I"$hb_dir/include" \
        -I"$build_dir" \
        -I"$source_dir/Include" \
        -I"$source_dir" \
        -I"$source_dir/Include/internal" \
        -o "$web_launcher" \
        "$root_dir/src/cpython_web_launcher.c" \
        "$root_dir/src/cpython_runtime.c" \
        "$root_dir/src/ps5_time.c" \
        "$root_dir/platform/cpython_ps5_host.c" \
        "$build_dir/Modules/config.o" \
        "$build_dir/libpython3.14.a" \
        -L"$hb_dir/lib" \
        -Wl,--wrap=clock_nanosleep -lmicrohttpd -ldl -lpthread
}

case "${1:-core}" in
    configure)
        configure_ps5
        ;;
    core)
        if [ ! -f "$build_dir/Makefile" ]; then
            configure_ps5
        fi
        CONFIG_SITE="$root_dir/tools/ps5.config.site" \
            make -C "$build_dir" -j"$jobs" Modules/config.o libpython3.14.a
        build_launcher
        build_runtime_bundle
        echo "Built $launcher"
        echo "Runtime bundle: $runtime_dir"
        ;;
    web)
        if [ ! -f "$build_dir/Makefile" ]; then
            configure_ps5
        fi
        CONFIG_SITE="$root_dir/tools/ps5.config.site" \
            make -C "$build_dir" -j"$jobs" Modules/config.o libpython3.14.a
        build_web_launcher
        build_runtime_bundle
        echo "Built $web_launcher"
        echo "Runtime bundle: $runtime_dir"
        ;;
    *)
        echo "Usage: $0 [configure|core|web]" >&2
        exit 2
        ;;
esac
