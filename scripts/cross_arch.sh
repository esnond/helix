#!/bin/sh
# cross_arch.sh - arm64 replay under qemu-user.
# needs gcc-aarch64-linux-gnu and qemu-user (apt install
# gcc-aarch64-linux-gnu qemu-user). builds the core for aarch64, runs
# the derive + dna roundtrip under qemu, compares the key with the
# native x86-64 build. a key mismatch is a release blocker.
set -eu
cd "$(dirname "$0")/.."
command -v aarch64-linux-gnu-gcc >/dev/null 2>&1 || {
    echo "BLOCKED_ENV: aarch64-linux-gnu-gcc not installed" >&2
    exit 3
}
command -v qemu-aarch64 >/dev/null 2>&1 || {
    echo "BLOCKED_ENV: qemu-aarch64 not installed" >&2
    exit 3
}
M="abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"
S=00112233445566778899aabbccddeeff
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
CF="-std=c99 -pedantic -Wall -Wextra -Werror -O2 -ffp-contract=off -D_DEFAULT_SOURCE -static"

make clean >/dev/null
CC=aarch64-linux-gnu-gcc make CC=aarch64-linux-gnu-gcc CFLAGS="$CF" \
    helix tests/test_dna >/dev/null 2>&1 || {
    echo "BLOCKED_ENV: aarch64 build failed" >&2
    make clean >/dev/null
    exit 3
}
mv helix "$T/helix.aarch64"
mv tests/test_dna "$T/test_dna.aarch64"

# native reference (rebuild for this machine)
make clean >/dev/null
make CFLAGS="$CF" helix >/dev/null 2>&1
native_key=$(LD_PRELOAD= ./helix derive -m "$M" -s $S | tail -1)

# arm64: derive
aarch64_key=$(LD_PRELOAD= qemu-aarch64 "$T/helix.aarch64" derive -m "$M" -s $S | tail -1)
echo "x86-64 $native_key"
echo "aarch64 $aarch64_key"
[ "$native_key" = "$aarch64_key" ] || {
    echo "CROSS_ARCH_MISMATCH" >&2
    make clean >/dev/null
    exit 1
}

# arm64: dna v2 + v3 roundtrips under qemu
echo "payload for the cross arch run" > "$T/in"
LD_PRELOAD= qemu-aarch64 "$T/helix.aarch64" encrypt "$T/in" "$T/v2.dna" \
    -m "$M" -g 3 >/dev/null
LD_PRELOAD= qemu-aarch64 "$T/helix.aarch64" decrypt "$T/v2.dna" "$T/v2.out" \
    -m "$M" >/dev/null
cmp -s "$T/in" "$T/v2.out" || { echo "CROSS_ARCH_V2_ROUNDTRIP_FAIL" >&2; exit 1; }
LD_PRELOAD= ./helix pair create "$T/p.hkpair" -p "cross arch" -k pbkdf2 >/dev/null
LD_PRELOAD= qemu-aarch64 "$T/helix.aarch64" encrypt "$T/in" "$T/v3.dna" \
    -m "$M" -g 3 -P "$T/p.hkpair" -p "cross arch" >/dev/null
LD_PRELOAD= qemu-aarch64 "$T/helix.aarch64" decrypt "$T/v3.dna" "$T/v3.out" \
    -m "$M" -P "$T/p.hkpair" -p "cross arch" >/dev/null
cmp -s "$T/in" "$T/v3.out" || { echo "CROSS_ARCH_V3_ROUNDTRIP_FAIL" >&2; exit 1; }

# cross validation: the x86-64 binary decrypts what arm64 encrypted
LD_PRELOAD= ./helix decrypt "$T/v2.dna" "$T/v2x.out" -m "$M" >/dev/null
cmp -s "$T/in" "$T/v2x.out" || { echo "CROSS_DECRYPT_FAIL" >&2; exit 1; }

# sanity: the aarch64 test_dna suite passes under qemu
LD_PRELOAD= qemu-aarch64 "$T/test_dna.aarch64" >/dev/null 2>&1 || {
    echo "CROSS_ARCH_TEST_DNA_FAIL" >&2
    exit 1
}
make clean >/dev/null
echo "cross-arch: identical keys, v2+v3 roundtrips, cross decrypt, test_dna ok"
