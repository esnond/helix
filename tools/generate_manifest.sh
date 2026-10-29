#!/bin/sh
# generate_manifest.sh - write manifest.txt for a results directory.
# usage: generate_manifest.sh <results_dir> <binary> [cli_flags]
# records UTC timestamp, binary name and SHA256, compiler, flags, seeds,
# duration, and the SHA256 of every CSV in the directory.
set -u

[ $# -ge 2 ] || { echo "usage: generate_manifest.sh <results_dir> <binary> [cli_flags]" >&2; exit 2; }
dir=$1
bin=$2
flags=${3:-unspecified}
[ -d "$dir" ] || { echo "generate_manifest.sh: no such directory: $dir" >&2; exit 2; }

hash_of() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | cut -d' ' -f1
    else
        shasum -a 256 "$1" | cut -d' ' -f1
    fi
}

manifest="$dir/manifest.txt"
{
    echo "# Generated: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "# Binary: $bin"
    if [ -f "$bin" ]; then
        echo "# Binary SHA256: $(hash_of "$bin")"
    else
        echo "# Binary SHA256: MISSING (binary not found at manifest time)"
    fi
    echo "# Compiler: $(cc --version 2>/dev/null | head -1 || echo unknown)"
    echo "# Flags: $flags"
    seeds=""
    n=0
    for f in "$dir"/seed_*.csv; do
        [ -f "$f" ] || continue
        s=$(basename "$f" .csv)
        seeds="$seeds ${s#seed_}"
        n=$((n + 1))
    done
    if [ "$n" -gt 0 ]; then
        echo "# Seeds: n=$n$seeds"
    else
        echo "# Seeds: unspecified"
    fi
    echo "# Duration: $(echo "$flags" | grep -oE -- '--duration[ =][0-9]+|-d [0-9]+' | head -1 | grep -oE '[0-9]+' || echo unspecified)"
    for f in "$dir"/*.csv; do
        [ -f "$f" ] || continue
        echo "$dir/$(basename "$f"): $(hash_of "$f")"
    done
} > "$manifest"
echo "manifest: $manifest"