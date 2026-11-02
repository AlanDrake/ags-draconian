#!/bin/sh
# Downloads the archives of the dependencies that CMake fetches (the ags_fetch_url calls in
# CMake/Fetch*.cmake) into a directory. Pass that directory to CMake as AGS_FETCH_MIRROR_DIR
# (a cache variable, or the environment variable of the same name) and the builds take the
# archives from there instead of the network; CMake still checks their hashes.
#
# Usage: Script/fetch_deps.sh <dir>
set -eu

dir=$1
repo=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$dir"

sed -n 's/^ags_fetch_url([A-Za-z0-9_]* \(https:[^)]*\)).*/\1/p' "$repo"/CMake/Fetch*.cmake | while read -r url; do
    archive="$dir/$(basename "$url")"
    if [ -s "$archive" ]; then
        echo "Have $(basename "$url")"
        continue
    fi
    echo "Downloading $url"
    # retry through short network outages, which are what this script is for
    curl --fail --location --silent --show-error --retry 8 --retry-delay 15 --retry-all-errors \
        --connect-timeout 30 --output "$archive.part" "$url"
    mv "$archive.part" "$archive"
done
