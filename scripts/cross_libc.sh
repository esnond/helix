#!/bin/sh
# cross_libc.sh - h1.1 cross-libc replay.
# needs musl-gcc (apt install musl-tools on debian). builds the same
# sources against musl and compares the derived key with the glibc one.
set -eu
cd "$(dirname "$0")/.."
command -v musl-gcc >/dev/null 2>&1 || {
    echo "BLOCKED_ENV: musl-gcc not installed (apt install musl-tools)" >&2
    exit 3
}
M="abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"
S=00112233445566778899aabbccddeeff
CF="-std=c99 -pedantic -Wall -Wextra -Werror -O2 -ffp-contract=off -D_DEFAULT_SOURCE"
make clean >/dev/null
CC=musl-gcc make CC=musl-gcc CFLAGS="$CF" >/dev/null
musl_key=$(LD_PRELOAD= ./helix derive -m "$M" -s $S | tail -1)
make clean >/dev/null
make CFLAGS="$CF" >/dev/null
glibc_key=$(LD_PRELOAD= ./helix derive -m "$M" -s $S | tail -1)
echo "musl  $musl_key"
echo "glibc $glibc_key"
[ "$musl_key" = "$glibc_key" ] || { echo "CROSS_LIBC_MISMATCH" >&2; exit 1; }
echo "cross-libc: identical"
