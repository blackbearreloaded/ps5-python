#!/usr/bin/env bash
set -eu

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "Usage: $0 PID [SIGNAL]" >&2
    exit 2
fi

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk_dir="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"
ps5_host="${PS5_HOST:-192.168.4.30}"
loader_port="${PS5_LOADER_PORT:-9021}"
remote_path="/data/python/runtime/ps5-kill.elf"
ftp_url="ftp://${ps5_host}:${PS5_FTP_PORT:-2121}"

bash "$root_dir/tools/build_ps5_kill.sh"
curl --fail --silent --show-error --upload-file \
    "$root_dir/build/ps5/ps5-kill.elf" "$ftp_url$remote_path"
source "$sdk_dir/toolchain/prospero.sh"

encoded_args="%2F${1}"
if [ "$#" -eq 2 ]; then
    encoded_args="${encoded_args}%20${2}"
fi
echo "Sending signal ${2:-15} to PS5 PID $1"
exec "$sdk_dir/bin/prospero-deploy" -h "$ps5_host" -p "$loader_port" \
    "file:$remote_path?args=$encoded_args"
