#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
source_dir="$root_dir/upstream/cpython"
build_dir="$root_dir/build/ps5"
build_python="$root_dir/build/host/python.exe"
sdk_dir="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"
hb_dir="$root_dir/build/ps5/deps/user/homebrew"
openssl_dir="$root_dir/build/ps5/deps/openssl"
jobs="${PS5_JOBS:-$(nproc 2>/dev/null || echo 2)}"
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

compiler=("$sdk_dir/bin/prospero-clang")
cache_tool="${PS5_CACHE:-auto}"
if [ "${PS5_CCACHE:-1}" = "0" ]; then
    cache_tool=none
fi
if [ "$cache_tool" = auto ]; then
    if command -v ccache >/dev/null 2>&1; then
        cache_tool=ccache
    elif command -v sccache >/dev/null 2>&1; then
        cache_tool=sccache
    else
        cache_tool=none
    fi
fi
if [ "$cache_tool" != none ]; then
    if ! command -v "$cache_tool" >/dev/null 2>&1; then
        echo "Requested compiler cache not found: $cache_tool" >&2
        exit 1
    fi
    compiler=("$cache_tool" "${compiler[@]}")
fi
linker_args=()
if [ "${PS5_LINKER:-lld}" = mold ]; then
    linker_args=(-fuse-ld=mold)
fi
compiler_string="${compiler[*]}"

needs_rebuild() {
    output="$1"
    shift
    [ ! -f "$output" ] && return 0
    for input in "$@"; do
        [ "$input" -nt "$output" ] && return 0
    done
    return 1
}

configure_ps5() {
    bash "$root_dir/tools/build_openssl_ps5.sh"
    mkdir -p "$build_dir"
    cd "$build_dir"
    CONFIG_SITE="$root_dir/tools/ps5.config.site" \
    CC="$compiler_string" \
    CPPFLAGS="-I$openssl_dir/include" \
    LDFLAGS="-L$openssl_dir/lib -lssl -lcrypto ${linker_args[*]}" \
    PKG_CONFIG_PATH="$openssl_dir/lib/pkgconfig" \
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
        --with-libm=no \
        --with-builtin-hashlib-hashes=
    # The SDK exposes timezone() as a function, not the POSIX timezone variable.
    sed -i 's/^#define HAVE_TZNAME 1$/#undef HAVE_TZNAME/' pyconfig.h
    sed -i 's/^#define HAVE_DECL_TZNAME 0$/#undef HAVE_DECL_TZNAME/' pyconfig.h
    cp "$root_dir/tools/ps5-setup.local" "$build_dir/Modules/Setup.local"
    printf '%s\n' \
        "_ssl _ssl.c -I$openssl_dir/include -L$openssl_dir/lib -lssl -lcrypto" \
        "_hashlib _hashopenssl.c -I$openssl_dir/include -L$openssl_dir/lib -lcrypto" \
        >> "$build_dir/Modules/Setup.local"
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
    mkdir -p "$runtime_dir/re" "$runtime_dir/json"
    cp "$source_dir/Lib/re/__init__.py" "$runtime_dir/re/__init__.py"
    for module in _casefix.py _compiler.py _constants.py _parser.py; do
        cp "$source_dir/Lib/re/$module" "$runtime_dir/re/$module"
    done
    for module in __init__.py decoder.py encoder.py scanner.py; do
        cp "$source_dir/Lib/json/$module" "$runtime_dir/json/$module"
    done

    # os is the Python-level POSIX wrapper; its native posix/time/stat pieces
    # are already compiled into Modules/config.c.
    cp "$root_dir/tools/minimal_selectors.py" "$runtime_dir/selectors.py"
    for module in os.py stat.py genericpath.py posixpath.py abc.py _collections_abc.py io.py socket.py enum.py types.py signal.py ssl.py base64.py warnings.py; do
        cp "$source_dir/Lib/$module" "$runtime_dir/$module"
    done
    cp "$source_dir/Lib/_py_warnings.py" "$runtime_dir/_py_warnings.py"
    # OpenSSL supplies the available digest implementations; the bundled
    # hashlib wrapper must not promise unavailable builtin BLAKE2 modules.
    sed "/'blake2b', 'blake2s',/d" "$source_dir/Lib/hashlib.py" \
        > "$runtime_dir/hashlib.py"
}

build_launcher() {
    if ! needs_rebuild "$launcher" \
        "$root_dir/src/cpython_runner.c" \
        "$root_dir/src/cpython_runtime.c" \
        "$root_dir/src/ps5_time.c" \
        "$root_dir/platform/cpython_ps5_host.c" \
        "$build_dir/Modules/config.o" \
        "$build_dir/libpython3.14.a"; then
        echo "Launcher unchanged: $launcher"
        return
    fi
    "${compiler[@]}" \
        "${linker_args[@]}" \
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
        -L"$openssl_dir/lib" -lssl -lcrypto \
        -Wl,--wrap=clock_nanosleep -ldl -lpthread
}

build_web_launcher() {
    bash "$root_dir/tools/build_libmicrohttpd.sh"
    if ! needs_rebuild "$web_launcher" \
        "$root_dir/src/cpython_web_launcher.c" \
        "$root_dir/src/cpython_runtime.c" \
        "$root_dir/src/ps5_time.c" \
        "$root_dir/platform/cpython_ps5_host.c" \
        "$build_dir/Modules/config.o" \
        "$build_dir/libpython3.14.a" \
        "$hb_dir/lib/libmicrohttpd.a"; then
        echo "Web launcher unchanged: $web_launcher"
        return
    fi
    "${compiler[@]}" \
        "${linker_args[@]}" \
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
        -L"$openssl_dir/lib" -lssl -lcrypto \
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
            CC="$compiler_string" make -C "$build_dir" -j"$jobs" Modules/config.o libpython3.14.a
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
            CC="$compiler_string" make -C "$build_dir" -j"$jobs" Modules/config.o libpython3.14.a
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
