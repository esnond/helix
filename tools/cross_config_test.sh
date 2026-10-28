#!/bin/sh
# h3 cross configuration determinism: build helix under multiple compiler
# configurations and compare the derived key for a fixed mnemonic and
# salt. writes results/cross_platform/platform_results.csv.
# expect: all x86_64 configs match; i386 sse matches (binary32 sse math,
# same libm); i386 x87 configurations expose the excess precision
# boundary documented in float_analysis.md.
set -e
cd "$(dirname "$0")/.."

M="abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"
S=00112233445566778899aabbccddeeff
OUT=results/cross_platform
CSV="$OUT/platform_results.csv"
mkdir -p "$OUT"
echo "config,key_hex,match_reference" > "$CSV"

BASE="-std=c99 -pedantic -Wall -Wextra -Werror -ffp-contract=off"

run_config() {
    name="$1"
    cflags="$2"
    make clean >/dev/null 2>&1
    if ! make CFLAGS="$BASE $cflags" helix >/dev/null 2>&1; then
        printf '%s,BUILD_FAIL,-\n' "$name" >> "$CSV"
        echo "config $name: BUILD_FAIL"
        return 0
    fi
    k=$(./helix derive -s "$S" -m "$M" 2>/dev/null)
    printf '%s,%s\n' "$name" "$k" >> "$CSV"
    echo "config $name: $k"
}

run_config x86_64-O2 "-O2"
run_config x86_64-O0 "-O0"
run_config x86_64-O1 "-O1"
run_config x86_64-O3 "-O3"
run_config x86_64-O3-native "-O3 -march=native"

# fill the reference column
key=$(sed -n '2p' "$CSV" | cut -d, -f2)
awk -F, -v ref="$key" 'NR>1 { if ($2 == ref) print $1","$2",yes"; else print $1","$2",no" }' \
    "$CSV" > "$CSV.tmp" && head -1 "$CSV" > "$CSV.new" && \
    grep -v '^config' "$CSV.tmp" >> "$CSV.new" && mv "$CSV.new" "$CSV" && rm "$CSV.tmp"

# 32 bit configurations (separate pass, they need the reference csv)
i386() {
    name="$1"
    cflags="$2"
    make clean >/dev/null 2>&1
    if ! make CFLAGS="$BASE $cflags" helix >/dev/null 2>&1; then
        printf '%s,BUILD_FAIL,-\n' "$name" >> "$CSV"
        echo "config $name: BUILD_FAIL"
        return 0
    fi
    k=$(./helix derive -s "$S" -m "$M" 2>/dev/null)
    match=no
    [ "$k" = "$key" ] && match=yes
    printf '%s,%s,%s\n' "$name" "$k" "$match" >> "$CSV"
    echo "config $name: $k (match=$match)"
}

i386 i386-sse "-m32 -msse2 -mfpmath=sse -O2"
i386 i386-x87 "-m32 -O2"
i386 i386-x87-excess "-m32 -O2 -fexcess-precision=standard"

make clean >/dev/null 2>&1
make >/dev/null 2>&1
echo "reference: $key"
echo "csv: $CSV"
