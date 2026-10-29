#!/bin/sh
# fuzz_dna.sh - nohup the .dna parser fuzzer under asan+ubsan, log to
# results/fuzz_dna/. usage: scripts/fuzz_dna.sh [iterations] [seed]
set -eu
cd "$(dirname "$0")/.."
iters=${1:-10000000}
seed=${2:-0x68757a7a6831}
make clean >/dev/null
LD_PRELOAD= make CFLAGS="-std=c99 -pedantic -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -ffp-contract=off" tools/fuzz_dna >/dev/null
mkdir -p results/fuzz_dna
nohup ./tools/fuzz_dna "$iters" "$seed" > "results/fuzz_dna/run_$(date -u +%Y%m%dT%H%M%SZ).log" 2>&1 &
echo "started pid $!"
