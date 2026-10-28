#!/bin/sh
# helix test gate. exits nonzero on first failure.
set -e
cd "$(dirname "$0")/.."

echo "test: wordlist integrity"
python3 - <<'EOF'
import hashlib
h = hashlib.sha256(open("data/bip39_english.txt", "rb").read()).hexdigest()
assert h == "2f5eed53a4727b4bf8880d8f3f199efc90e58503646d9ff8eff3a2ed3b24dbda", h
words = open("data/bip39_english.txt").read().split()
assert len(words) == 2048 and len(set(words)) == 2048
EOF

echo "test: generated headers reproducible"
python3 tools/gen_vectors.py --check

for t in tests/test_*; do
    case "$t" in
    *.c|*.o|*.h) continue ;;
    esac
    echo "test: $(basename "$t")"
    ./"$t" || exit 1
done

echo "test: shell determinism, two derive runs bitwise identical"
M="abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"
./helix derive -s 00112233445566778899aabbccddeeff -m "$M" > /tmp/helix_d1.$$
./helix derive -s 00112233445566778899aabbccddeeff -m "$M" > /tmp/helix_d2.$$
cmp /tmp/helix_d1.$$ /tmp/helix_d2.$$
rm -f /tmp/helix_d1.$$ /tmp/helix_d2.$$
echo "shell determinism: ok"

echo "test: shell evolution determinism, two fresh keychains"
KC1=/tmp/helix_shell_kc1.$$.db
KC2=/tmp/helix_shell_kc2.$$.db
G1=$(./helix evolve -k "$KC1" -m "$M")
G2=$(./helix evolve -k "$KC2" -m "$M")
[ "$G1" = "$G2" ] || { echo "FAIL: evolution sequences differ"; exit 1; }
G3=$(./helix evolve -k "$KC1" -m "$M")
G4=$(./helix evolve -k "$KC2" -m "$M")
[ "$G3" = "$G4" ] || { echo "FAIL: gen 2 sequences differ"; exit 1; }
[ "${G3%% *}" = "2" ] || { echo "FAIL: expected gen 2, got $G3"; exit 1; }
rm -f "$KC1" "$KC2"
echo "shell evolution determinism: ok"

echo "test: cli dna roundtrip (1 mb random file)"
head -c 1048576 /dev/urandom > /tmp/helix_cli.bin
./helix encrypt /tmp/helix_cli.bin /tmp/helix_cli.dna -m "$M" -g 0
./helix decrypt /tmp/helix_cli.dna /tmp/helix_cli.out -m "$M"
cmp /tmp/helix_cli.bin /tmp/helix_cli.out
./helix decrypt /tmp/helix_cli.dna /tmp/helix_cli.out -m "$M" \
    -k /tmp/helix_no_such_kch.db
cmp /tmp/helix_cli.bin /tmp/helix_cli.out
if ./helix decrypt /tmp/helix_cli.dna /tmp/helix_cli.out \
    -m "abandon abandon abandon" 2>/dev/null; then
    echo "FAIL: bad mnemonic decrypt must fail"; exit 1
fi
rm -f /tmp/helix_cli.bin /tmp/helix_cli.dna /tmp/helix_cli.out
echo "cli dna roundtrip: ok"

echo "tests: ok"
