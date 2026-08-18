#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk_dir="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"
ps5_host="${PS5_HOST:-192.168.4.30}"
ftp_port="${PS5_FTP_PORT:-2121}"
loader_port="${PS5_LOADER_PORT:-9021}"
web_port="${PS5_WEB_PORT:-8090}"
ftp_url="ftp://$ps5_host:$ftp_port"

bash "$root_dir/tools/build_ps5.sh" web

web_elf="$root_dir/build/ps5/python-web.elf"
runtime_dir="$root_dir/build/ps5/cpython-lib"
web_dir="$root_dir/web"
apps_dir="$root_dir/apps"

mkdir_remote() {
    curl --silent --show-error -Q "MKD $1" "$ftp_url/" >/dev/null 2>&1 || true
}

upload() {
    curl --fail --silent --show-error --upload-file "$1" "$ftp_url$2"
}

mkdir_remote /data/python
mkdir_remote /data/python/web
mkdir_remote /data/python/runtime
mkdir_remote /data/python/runtime/cpython-lib
mkdir_remote /data/python/runtime/cpython-lib/encodings
mkdir_remote /data/python/apps
upload "$web_elf" /data/python/runtime/python-web.elf
upload "$web_dir/index.html" /data/python/web/index.html
upload "$web_dir/app.css" /data/python/web/app.css
upload "$web_dir/app.js" /data/python/web/app.js
upload "$runtime_dir/codecs.py" /data/python/runtime/cpython-lib/codecs.py
upload "$runtime_dir/encodings/__init__.py" /data/python/runtime/cpython-lib/encodings/__init__.py
upload "$runtime_dir/encodings/ascii.py" /data/python/runtime/cpython-lib/encodings/ascii.py
upload "$runtime_dir/encodings/utf_8.py" /data/python/runtime/cpython-lib/encodings/utf_8.py
upload "$runtime_dir/encodings/idna.py" /data/python/runtime/cpython-lib/encodings/idna.py
upload "$runtime_dir/selectors.py" /data/python/runtime/cpython-lib/selectors.py
for module in os.py stat.py genericpath.py posixpath.py abc.py _collections_abc.py io.py socket.py enum.py types.py signal.py hashlib.py ssl.py base64.py warnings.py contextvars.py _py_warnings.py _weakrefset.py tracemalloc.py csv.py decimal.py numbers.py contextlib.py weakref.py copy.py copyreg.py _compat_pickle.py hmac.py random.py bisect.py glob.py fnmatch.py functools.py operator.py reprlib.py linecache.py pickle.py struct.py timeit.py dis.py opcode.py _opcode_metadata.py; do
    upload "$runtime_dir/$module" "/data/python/runtime/cpython-lib/$module"
done
for module in threading.py queue.py runpy.py secrets.py tempfile.py datetime.py typing.py annotationlib.py ast.py keyword.py __future__.py argparse.py gettext.py locale.py traceback.py pprint.py textwrap.py codeop.py tokenize.py token.py _colorize.py difflib.py inspect.py calendar.py quopri.py ipaddress.py subprocess.py shutil.py; do
    upload "$runtime_dir/$module" "/data/python/runtime/cpython-lib/$module"
done
for package in logging string urllib http email unittest; do
    mkdir_remote "/data/python/runtime/cpython-lib/$package"
    while IFS= read -r -d '' module_file; do
        relative_file="${module_file#"$runtime_dir/$package/"}"
        remote_file="/data/python/runtime/cpython-lib/$package/$relative_file"
        mkdir_remote "${remote_file%/*}"
        upload "$module_file" "$remote_file"
    done < <(find "$runtime_dir/$package" -type f -name '*.py' -print0 | sort -z)
done
mkdir_remote /data/python/runtime/cpython-lib/importlib
for module in __init__.py _abc.py machinery.py util.py; do
    upload "$runtime_dir/importlib/$module" "/data/python/runtime/cpython-lib/importlib/$module"
done
for package in concurrent multiprocessing; do
    mkdir_remote "/data/python/runtime/cpython-lib/$package"
    if [ "$package" = concurrent ]; then
        mkdir_remote "/data/python/runtime/cpython-lib/$package/futures"
    fi
    while IFS= read -r -d '' module_file; do
        relative_file="${module_file#"$runtime_dir/$package/"}"
        remote_file="/data/python/runtime/cpython-lib/$package/$relative_file"
        mkdir_remote "${remote_file%/*}"
        upload "$module_file" "$remote_file"
    done < <(find "$runtime_dir/$package" -type f -name '*.py' -print0 | sort -z)
done
mkdir_remote /data/python/runtime/cpython-lib/xml
mkdir_remote /data/python/runtime/cpython-lib/xml/etree
upload "$runtime_dir/xml/__init__.py" /data/python/runtime/cpython-lib/xml/__init__.py
upload "$runtime_dir/xml/etree/__init__.py" /data/python/runtime/cpython-lib/xml/etree/__init__.py
upload "$runtime_dir/xml/etree/ElementTree.py" /data/python/runtime/cpython-lib/xml/etree/ElementTree.py
upload "$runtime_dir/xml/etree/ElementPath.py" /data/python/runtime/cpython-lib/xml/etree/ElementPath.py
mkdir_remote /data/python/runtime/cpython-lib/pathlib
for module in __init__.py _local.py _os.py types.py; do
    upload "$runtime_dir/pathlib/$module" "/data/python/runtime/cpython-lib/pathlib/$module"
done
upload "$runtime_dir/zipimport.py" /data/python/runtime/cpython-lib/zipimport.py
mkdir_remote /data/python/runtime/cpython-lib/collections
upload "$runtime_dir/collections/__init__.py" /data/python/runtime/cpython-lib/collections/__init__.py
upload "$runtime_dir/heapq.py" /data/python/runtime/cpython-lib/heapq.py
upload "$runtime_dir/dataclasses.py" /data/python/runtime/cpython-lib/dataclasses.py
mkdir_remote /data/python/runtime/cpython-lib/ctypes
for module in __init__.py _endian.py _layout.py _aix.py util.py wintypes.py; do
    upload "$runtime_dir/ctypes/$module" "/data/python/runtime/cpython-lib/ctypes/$module"
done
upload "$runtime_dir/sysconfig.py" /data/python/runtime/cpython-lib/sysconfig.py
for package in re json; do
    mkdir_remote "/data/python/runtime/cpython-lib/$package"
    while IFS= read -r -d '' module_file; do
        upload "$module_file" "/data/python/runtime/cpython-lib/$package/$(basename "$module_file")"
    done < <(find "$runtime_dir/$package" -maxdepth 1 -type f -name '*.py' -print0 | sort -z)
done

while IFS= read -r -d '' app_file; do
    relative_file="${app_file#"$apps_dir/"}"
    remote_file="/data/python/apps/$relative_file"
    mkdir_remote "${remote_file%/*}"
    upload "$app_file" "$remote_file"
done < <(find "$apps_dir" -type f \
    -not -path '*/__pycache__/*' \
    -not -name '*.pyc' \
    -print0 | sort -z)

source "$sdk_dir/toolchain/prospero.sh"
launch_uri="file:/data/python/runtime/python-web.elf?args=$web_port"
log_file="$root_dir/build/ps5/python-web-deploy.log"
nohup "$sdk_dir/bin/prospero-deploy" -h "$ps5_host" -p "$loader_port" \
    "$launch_uri" >"$log_file" 2>&1 < /dev/null &
deploy_pid=$!

ready=0
for attempt in $(seq 1 20); do
    if curl --fail --silent --show-error \
        "http://$ps5_host:$web_port/api/status" >/dev/null 2>&1; then
        ready=1
        break
    fi
    sleep 1
done

if [ "$ready" -ne 1 ]; then
    echo "Python web launcher did not become ready." >&2
    cat "$log_file" >&2 || true
    kill "$deploy_pid" >/dev/null 2>&1 || true
    exit 1
fi

echo "Python web launcher: http://$ps5_host:$web_port/"

if [ "${PS5_WEB_CHECK:-0}" = "1" ]; then
    echo "Apps:"
    curl --fail --silent --show-error "http://$ps5_host:$web_port/api/apps"
    echo
    curl --fail --silent --show-error \
        "http://$ps5_host:$web_port/api/launch?app=hello"
    echo
    for attempt in $(seq 1 20); do
        logs=$(curl --fail --silent --show-error \
            "http://$ps5_host:$web_port/api/logs?since=0")
        if printf '%s' "$logs" | grep -q "Hello from a packaged Python app on PS5"; then
            echo "Live app output: PASS"
            break
        fi
        sleep 1
    done
    printf '%s\n' "$logs"
    curl --fail --silent --show-error \
        "http://$ps5_host:$web_port/api/shutdown" >/dev/null
    wait "$deploy_pid" || true
fi
