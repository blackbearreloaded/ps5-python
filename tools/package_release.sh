#!/usr/bin/env bash
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tag="${1:-}"
output_dir="${2:-$root_dir/dist}"
build_dir="${PS5_BUILD_DIR:-$root_dir/build/ps5}"

if [ -z "$tag" ]; then
    echo "Usage: $0 TAG [OUTPUT_DIR]" >&2
    exit 2
fi
case "$tag" in
    *[!A-Za-z0-9._-]*)
        echo "Release tag contains unsupported path characters: $tag" >&2
        exit 2
        ;;
esac

for artifact in python.elf python-web.elf python-app-supervisor.elf; do
    if [ ! -f "$build_dir/$artifact" ]; then
        echo "Missing $build_dir/$artifact; build the PS5 artifacts first." >&2
        exit 1
    fi
done
if [ ! -d "$build_dir/cpython-lib" ]; then
    echo "Missing $build_dir/cpython-lib; build the PS5 runtime first." >&2
    exit 1
fi

mkdir -p "$output_dir"
staging_dir=$(mktemp -d)
trap 'rm -rf "$staging_dir"' EXIT
bundle_dir="$staging_dir/python-ps5-$tag"
mkdir -p "$bundle_dir/runtime"

cp "$build_dir/python.elf" \
    "$output_dir/python-ps5-$tag-python.elf"
cp "$build_dir/python-web.elf" \
    "$output_dir/python-ps5-$tag-python-web.elf"
cp "$build_dir/python-app-supervisor.elf" \
    "$output_dir/python-ps5-$tag-python-app-supervisor.elf"
cp "$build_dir/python.elf" "$bundle_dir/runtime/python.elf"
cp "$build_dir/python-web.elf" "$bundle_dir/runtime/python-web.elf"
cp "$build_dir/python-app-supervisor.elf" \
    "$bundle_dir/runtime/python-app-supervisor.elf"
cp -a "$build_dir/cpython-lib" "$bundle_dir/runtime/cpython-lib"
cp -a "$root_dir/web" "$bundle_dir/web"
cp -a "$root_dir/apps" "$bundle_dir/apps"

archive="$output_dir/python-ps5-$tag.tar.gz"
tar --exclude='__pycache__' --exclude='*.pyc' \
    -czf "$archive" -C "$staging_dir" "python-ps5-$tag"

(
    cd "$output_dir"
    sha256sum \
        "python-ps5-$tag.tar.gz" \
        "python-ps5-$tag-python.elf" \
        "python-ps5-$tag-python-web.elf" \
        "python-ps5-$tag-python-app-supervisor.elf"
) > "$output_dir/python-ps5-$tag-SHA256SUMS"

echo "Release assets written to $output_dir"
