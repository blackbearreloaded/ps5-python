#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
script_arg="${1:-examples/main.py}"
script_path="$script_arg"
app_dir="${PS5_APP_DIR:-}"
sdk_dir="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"
ps5_host="${PS5_HOST:-192.168.4.30}"
ftp_port="${PS5_FTP_PORT:-2121}"
loader_port="${PS5_LOADER_PORT:-9021}"
run_timeout="${RUN_TIMEOUT:-20}"
remote_root="${PS5_RUNTIME_ROOT:-/data/python}"
remote_root="${remote_root%/}"

case "$remote_root" in
    ''|/*) ;;
    *)
        echo "PS5_RUNTIME_ROOT must be an absolute PS5 path." >&2
        exit 2
        ;;
esac

remote_elf="$remote_root/python.elf"
remote_script="$remote_root/main.py"
remote_runtime="$remote_root/cpython-lib"
remote_app_root=""
remote_app_lib=""

if [ -n "$app_dir" ]; then
    if [ "${app_dir#/}" = "$app_dir" ]; then
        app_dir="$root_dir/$app_dir"
    fi
    if [ ! -f "$app_dir/app.json" ]; then
        echo "Missing app manifest: $app_dir/app.json" >&2
        exit 1
    fi

    app_id=$(basename "$app_dir")
    entry_path=$(sed -n 's/.*"entry"[[:space:]]*:[[:space:]]*"\([^"/][^"]*\)".*/\1/p' "$app_dir/app.json")
    if [ -z "$entry_path" ]; then
        echo "Manifest must define a relative entry path: $app_dir/app.json" >&2
        exit 1
    fi
    case "$entry_path" in
        /*|*..*)
            echo "Manifest entry must stay inside the app bundle: $entry_path" >&2
            exit 1
            ;;
    esac

    script_path="$app_dir/$entry_path"
    if [ ! -f "$script_path" ]; then
        echo "Missing app entry: $script_path" >&2
        exit 1
    fi

    remote_app_root="$remote_root/apps/$app_id"
    remote_app_lib="$remote_app_root/lib"
    remote_elf="$remote_root/runtime/python.elf"
    remote_script="$remote_app_root/$entry_path"
    remote_runtime="$remote_root/runtime/cpython-lib"
fi

if [ "${script_path#/}" = "$script_path" ]; then
    script_path="$root_dir/$script_path"
fi

if [ ! -f "$script_path" ]; then
    echo "Missing script: $script_path" >&2
    exit 1
fi

bash "$root_dir/tools/build_ps5.sh" core

elf="$root_dir/build/ps5/python.elf"
runtime_dir="$root_dir/build/ps5/cpython-lib"
ftp_url="ftp://$ps5_host:$ftp_port"

mkdir_remote() {
    curl --silent --show-error -Q "MKD $1" "$ftp_url/" >/dev/null 2>&1 || true
}

upload() {
    curl --fail --silent --show-error --upload-file "$1" "$ftp_url$2"
}

mkdir_remote "$remote_root"
if [ -n "$remote_app_root" ]; then
    mkdir_remote "$remote_root/runtime"
fi
mkdir_remote "$remote_runtime"
mkdir_remote "$remote_runtime/encodings"
if [ -n "$remote_app_root" ]; then
    mkdir_remote "$remote_root/apps"
    mkdir_remote "$remote_app_root"
fi

upload "$elf" "$remote_elf"
upload "$script_path" "$remote_script"
upload "$runtime_dir/codecs.py" "$remote_runtime/codecs.py"
upload "$runtime_dir/site.py" "$remote_runtime/site.py"
upload "$runtime_dir/_sitebuiltins.py" "$remote_runtime/_sitebuiltins.py"
upload "$runtime_dir/encodings/__init__.py" "$remote_runtime/encodings/__init__.py"
upload "$runtime_dir/encodings/ascii.py" "$remote_runtime/encodings/ascii.py"
upload "$runtime_dir/encodings/aliases.py" "$remote_runtime/encodings/aliases.py"
upload "$runtime_dir/encodings/utf_8.py" "$remote_runtime/encodings/utf_8.py"
upload "$runtime_dir/encodings/cp437.py" "$remote_runtime/encodings/cp437.py"
upload "$runtime_dir/encodings/idna.py" "$remote_runtime/encodings/idna.py"
upload "$runtime_dir/selectors.py" "$remote_runtime/selectors.py"
for module in os.py stat.py genericpath.py posixpath.py abc.py _collections_abc.py io.py socket.py enum.py types.py signal.py hashlib.py ssl.py base64.py warnings.py contextvars.py _py_warnings.py _weakrefset.py tracemalloc.py csv.py decimal.py fractions.py numbers.py contextlib.py weakref.py copy.py copyreg.py _compat_pickle.py hmac.py random.py bisect.py glob.py fnmatch.py functools.py operator.py reprlib.py linecache.py pickle.py struct.py timeit.py dis.py opcode.py _opcode_metadata.py gzip.py tarfile.py uuid.py filecmp.py tty.py; do
    upload "$runtime_dir/$module" "$remote_runtime/$module"
done
for module in threading.py queue.py runpy.py secrets.py getpass.py tempfile.py datetime.py typing.py annotationlib.py ast.py _ast_unparse.py keyword.py __future__.py argparse.py gettext.py locale.py traceback.py pprint.py textwrap.py codeop.py tokenize.py token.py _colorize.py difflib.py inspect.py calendar.py quopri.py ipaddress.py socketserver.py mimetypes.py subprocess.py shutil.py; do
    upload "$runtime_dir/$module" "$remote_runtime/$module"
done
for package in logging string urllib http email unittest asyncio html compression zipfile xml sqlite3 sysconfig; do
    mkdir_remote "$remote_runtime/$package"
    while IFS= read -r -d '' module_file; do
        relative_file="${module_file#"$runtime_dir/$package/"}"
        remote_file="$remote_runtime/$package/$relative_file"
        mkdir_remote "${remote_file%/*}"
        upload "$module_file" "$remote_file"
    done < <(find "$runtime_dir/$package" -type f -name '*.py' -print0 | sort -z)
done
mkdir_remote "$remote_runtime/importlib"
for module in __init__.py _abc.py abc.py machinery.py util.py; do
    upload "$runtime_dir/importlib/$module" "$remote_runtime/importlib/$module"
done
for package in concurrent multiprocessing; do
    mkdir_remote "$remote_runtime/$package"
    if [ "$package" = concurrent ]; then
        mkdir_remote "$remote_runtime/$package/futures"
    fi
    while IFS= read -r -d '' module_file; do
        relative_file="${module_file#"$runtime_dir/$package/"}"
        remote_file="$remote_runtime/$package/$relative_file"
        mkdir_remote "${remote_file%/*}"
        upload "$module_file" "$remote_file"
    done < <(find "$runtime_dir/$package" -type f -name '*.py' -print0 | sort -z)
done
mkdir_remote "$remote_runtime/pathlib"
for module in __init__.py _local.py _os.py types.py; do
    upload "$runtime_dir/pathlib/$module" "$remote_runtime/pathlib/$module"
done
upload "$runtime_dir/zipimport.py" "$remote_runtime/zipimport.py"
mkdir_remote "$remote_runtime/collections"
upload "$runtime_dir/collections/__init__.py" "$remote_runtime/collections/__init__.py"
upload "$runtime_dir/heapq.py" "$remote_runtime/heapq.py"
upload "$runtime_dir/dataclasses.py" "$remote_runtime/dataclasses.py"
mkdir_remote "$remote_runtime/ctypes"
for module in __init__.py _endian.py _layout.py _aix.py util.py wintypes.py; do
    upload "$runtime_dir/ctypes/$module" "$remote_runtime/ctypes/$module"
done
for package in re json; do
    mkdir_remote "$remote_runtime/$package"
    while IFS= read -r -d '' module_file; do
        upload "$module_file" "$remote_runtime/$package/$(basename "$module_file")"
    done < <(find "$runtime_dir/$package" -maxdepth 1 -type f -name '*.py' -print0 | sort -z)
done

if [ -n "$remote_app_root" ]; then
    while IFS= read -r -d '' app_file; do
        relative_file="${app_file#"$app_dir/"}"
        remote_file="$remote_app_root/$relative_file"
        mkdir_remote "${remote_file%/*}"
        upload "$app_file" "$remote_file"
    done < <(find "$app_dir" -type f \
        -not -path '*/__pycache__/*' \
        -not -name '*.pyc' \
        -print0 | sort -z)
fi

if [ "$script_path" = "$root_dir/tests/core_suite.py" ]; then
    mkdir_remote "$remote_root/core-tests"
    for test_file in \
        "$root_dir"/tests/core_basics.py \
        "$root_dir"/tests/core_control/*.py \
        "$root_dir"/tests/core_objects/classes/*.py \
        "$root_dir"/tests/core_objects/functions/*.py \
        "$root_dir"/tests/core_objects/iteration/*.py \
        "$root_dir"/tests/core_values/*.py \
        "$root_dir"/tests/stdlib/*.py; do
        upload "$test_file" "$remote_root/core-tests/$(basename "$test_file")"
    done
fi

encoded_script="${remote_script#/}"
encoded_script="${encoded_script//\//%2F}"
encoded_runtime="${remote_runtime#/}"
encoded_runtime="${encoded_runtime//\//%2F}"
encoded_args="%2F${encoded_script}%20%2F${encoded_runtime}"
if [ -n "$remote_app_root" ]; then
    encoded_app_root="${remote_app_root#/}"
    encoded_app_root="${encoded_app_root//\//%2F}"
    encoded_app_lib="${remote_app_lib#/}"
    encoded_app_lib="${encoded_app_lib//\//%2F}"
    encoded_args="${encoded_args}%20%2F${encoded_app_root}%20%2F${encoded_app_lib}"
fi

source "$sdk_dir/toolchain/prospero.sh"

echo "Launching python.elf with $script_arg"
set +e
timeout "$run_timeout" "$sdk_dir/bin/prospero-deploy" \
    -h "$ps5_host" -p "$loader_port" \
    "file:$remote_elf?args=$encoded_args"
status=$?
set -e

if [ "$status" -eq 124 ]; then
    echo "Launcher timed out after ${run_timeout}s" >&2
    exit 124
fi
exit "$status"
