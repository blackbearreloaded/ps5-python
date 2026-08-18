#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
source_dir="$root_dir/upstream/cpython"
build_dir="$root_dir/build/ps5"
build_python="$root_dir/build/host/python.exe"
sdk_dir="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"
hb_dir="$root_dir/build/ps5/deps/user/homebrew"
openssl_dir="$root_dir/build/ps5/deps/openssl"
libffi_dir="$root_dir/build/ps5/deps/libffi"
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
    bash "$root_dir/tools/build_libffi_ps5.sh"
    mkdir -p "$build_dir"
    cd "$build_dir"
    CONFIG_SITE="$root_dir/tools/ps5.config.site" \
    CC="$compiler_string" \
    CPPFLAGS="-I$openssl_dir/include -I$libffi_dir/include" \
    LDFLAGS="-L$openssl_dir/lib -lssl -lcrypto -L$libffi_dir/lib -lffi ${linker_args[*]}" \
    LIBFFI_CFLAGS="-I$libffi_dir/include" \
    LIBFFI_LIBS="-L$libffi_dir/lib -lffi" \
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
        "pyexpat pyexpat.c -I$source_dir/Modules/expat" \
        "_elementtree _elementtree.c -I$source_dir/Modules/expat" \
        "_ctypes _ctypes/_ctypes.c _ctypes/callbacks.c _ctypes/callproc.c _ctypes/stgdict.c _ctypes/cfield.c -I$libffi_dir/include -L$libffi_dir/lib -lffi" \
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
    cp "$source_dir/Lib/encodings/aliases.py" \
        "$runtime_dir/encodings/aliases.py"
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
    for module in os.py stat.py genericpath.py posixpath.py abc.py _collections_abc.py io.py socket.py enum.py types.py signal.py ssl.py base64.py warnings.py contextvars.py numbers.py contextlib.py weakref.py copy.py copyreg.py _compat_pickle.py hmac.py random.py bisect.py glob.py fnmatch.py functools.py operator.py reprlib.py linecache.py pickle.py struct.py timeit.py dis.py opcode.py; do
        cp "$source_dir/Lib/$module" "$runtime_dir/$module"
    done
    cp "$source_dir/Lib/_opcode_metadata.py" "$runtime_dir/_opcode_metadata.py"
    cp "$source_dir/Lib/_py_warnings.py" "$runtime_dir/_py_warnings.py"
    cp "$source_dir/Lib/_weakrefset.py" "$runtime_dir/_weakrefset.py"
    cp "$source_dir/Lib/tracemalloc.py" "$runtime_dir/tracemalloc.py"
    cp "$source_dir/Lib/threading.py" "$runtime_dir/threading.py"
    cp "$source_dir/Lib/queue.py" "$runtime_dir/queue.py"
    rm -rf "$runtime_dir/logging"
    mkdir -p "$runtime_dir/logging"
    for module in "$source_dir"/Lib/logging/*.py; do
        cp "$module" "$runtime_dir/logging/$(basename "$module")"
    done
    for module in __future__.py argparse.py gettext.py locale.py traceback.py pprint.py textwrap.py codeop.py tokenize.py token.py _colorize.py difflib.py inspect.py calendar.py quopri.py ipaddress.py; do
        cp "$source_dir/Lib/$module" "$runtime_dir/$module"
    done
    mkdir -p "$runtime_dir/string"
    cp "$source_dir/Lib/string/__init__.py" "$runtime_dir/string/__init__.py"
    "$build_python" "$root_dir/tools/patch_subprocess.py" \
        "$source_dir/Lib/subprocess.py" \
        "$runtime_dir/subprocess.py"
    mkdir -p "$runtime_dir/urllib"
    for module in __init__.py error.py parse.py request.py response.py robotparser.py; do
        cp "$source_dir/Lib/urllib/$module" "$runtime_dir/urllib/$module"
    done
    mkdir -p "$runtime_dir/http"
    for module in __init__.py client.py cookiejar.py cookies.py server.py; do
        cp "$source_dir/Lib/http/$module" "$runtime_dir/http/$module"
    done
    mkdir -p "$runtime_dir/email"
    for module in "$source_dir"/Lib/email/*.py; do
        cp "$module" "$runtime_dir/email/$(basename "$module")"
    done
    mkdir -p "$runtime_dir/unittest"
    for module in "$source_dir"/Lib/unittest/*.py; do
        cp "$module" "$runtime_dir/unittest/$(basename "$module")"
    done
    cp "$source_dir/Lib/pprint.py" "$runtime_dir/pprint.py"
    cp "$source_dir/Lib/runpy.py" "$runtime_dir/runpy.py"
    cp "$source_dir/Lib/secrets.py" "$runtime_dir/secrets.py"
    cp "$source_dir/Lib/tempfile.py" "$runtime_dir/tempfile.py"
    cp "$source_dir/Lib/datetime.py" "$runtime_dir/datetime.py"
    cp "$source_dir/Lib/typing.py" "$runtime_dir/typing.py"
    cp "$source_dir/Lib/annotationlib.py" "$runtime_dir/annotationlib.py"
    cp "$source_dir/Lib/ast.py" "$runtime_dir/ast.py"
    cp "$source_dir/Lib/keyword.py" "$runtime_dir/keyword.py"
    "$build_python" "$root_dir/tools/patch_shutil_rmtree.py" \
        "$source_dir/Lib/shutil.py" \
        "$runtime_dir/shutil.py"
    mkdir -p "$runtime_dir/importlib"
    for module in __init__.py _abc.py machinery.py util.py; do
        cp "$source_dir/Lib/importlib/$module" "$runtime_dir/importlib/$module"
    done
    mkdir -p "$runtime_dir/concurrent/futures"
    cp "$source_dir/Lib/concurrent/__init__.py" "$runtime_dir/concurrent/__init__.py"
    cp "$source_dir/Lib/concurrent/futures/__init__.py" "$runtime_dir/concurrent/futures/__init__.py"
    cp "$source_dir/Lib/concurrent/futures/_base.py" "$runtime_dir/concurrent/futures/_base.py"
    cp "$source_dir/Lib/concurrent/futures/thread.py" "$runtime_dir/concurrent/futures/thread.py"
    mkdir -p "$runtime_dir/multiprocessing"
    for module in "$source_dir"/Lib/multiprocessing/*.py; do
        cp "$module" "$runtime_dir/multiprocessing/$(basename "$module")"
    done
    "$build_python" "$root_dir/tools/patch_multiprocessing_util.py" \
        "$source_dir/Lib/multiprocessing/util.py" \
        "$runtime_dir/multiprocessing/util.py"
    cp "$source_dir/Lib/csv.py" "$runtime_dir/csv.py"
    cp "$source_dir/Lib/decimal.py" "$runtime_dir/decimal.py"
    mkdir -p "$runtime_dir/xml/etree"
    cp "$source_dir/Lib/xml/__init__.py" "$runtime_dir/xml/__init__.py"
    cp "$source_dir/Lib/xml/etree/__init__.py" "$runtime_dir/xml/etree/__init__.py"
    cp "$source_dir/Lib/xml/etree/ElementTree.py" "$runtime_dir/xml/etree/ElementTree.py"
    cp "$source_dir/Lib/xml/etree/ElementPath.py" "$runtime_dir/xml/etree/ElementPath.py"
    mkdir -p "$runtime_dir/pathlib"
    for module in __init__.py _local.py _os.py types.py; do
        cp "$source_dir/Lib/pathlib/$module" "$runtime_dir/pathlib/$module"
    done
    cp "$source_dir/Lib/zipimport.py" "$runtime_dir/zipimport.py"
    mkdir -p "$runtime_dir/collections"
    cp "$source_dir/Lib/collections/__init__.py" "$runtime_dir/collections/__init__.py"
    cp "$source_dir/Lib/heapq.py" "$runtime_dir/heapq.py"
    cp "$root_dir/tools/minimal_dataclasses.py" "$runtime_dir/dataclasses.py"
    rm -rf "$runtime_dir/ctypes" "$runtime_dir/sysconfig"
    cp "$root_dir/tools/minimal_sysconfig.py" "$runtime_dir/sysconfig.py"
    mkdir -p "$runtime_dir/ctypes"
    for module in __init__.py _endian.py _layout.py _aix.py util.py wintypes.py; do
        if [ "$module" = __init__.py ]; then
            sed -e '/import sysconfig as _sysconfig/d' \
                -e 's/_sysconfig.get_config_var("LDLIBRARY")/None/' \
                "$source_dir/Lib/ctypes/$module" > "$runtime_dir/ctypes/$module"
        else
            cp "$source_dir/Lib/ctypes/$module" "$runtime_dir/ctypes/$module"
        fi
    done
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
        "$build_dir/libpython3.14.a" \
        "$build_dir/Modules/expat/libexpat.a" \
        "$build_dir/Modules/_decimal/libmpdec/libmpdec.a"; then
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
        "$build_dir/Modules/expat/libexpat.a" \
        "$build_dir/Modules/_decimal/libmpdec/libmpdec.a" \
        -L"$openssl_dir/lib" -lssl -lcrypto \
        -L"$libffi_dir/lib" -lffi \
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
        "$build_dir/Modules/expat/libexpat.a" \
        "$build_dir/Modules/_decimal/libmpdec/libmpdec.a" \
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
        "$build_dir/Modules/expat/libexpat.a" \
        "$build_dir/Modules/_decimal/libmpdec/libmpdec.a" \
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
