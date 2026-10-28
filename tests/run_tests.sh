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

echo "tests: ok"
