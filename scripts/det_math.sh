#!/bin/sh
# det_math.sh - h1.1 criterion 5: the derived key must not depend on the
# optimization level once libm is out of the path.
set -eu
cd "$(dirname "$0")/.."
M="abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"
S=00112233445566778899aabbccddeeff
base="-std=c99 -pedantic -Wall -Wextra -Werror -ffp-contract=off"
for opt in -O0 -O1 -O2 -O3; do
    make clean >/dev/null
    make CFLAGS="$base $opt" >/dev/null
    key=$(./helix derive -m "$M" -s $S | tail -1)
    echo "$opt $key"
done
make clean >/dev/null
make >/dev/null
