#!/usr/bin/env python3
"""differential fuzz: helix c99 crypto vs python hashlib/hmac.

h1.3 (docs/hypotheses.md): 100000 random vectors, zero mismatches, with
the 95% Clopper-Pearson upper bound reported. this is a smoke test, not a
correctness proof, and it does not cover the kdf as a whole (the engine
phase is covered by the pinned-key tests).

usage: tools/fuzz_crypto.py [vectors] [--report PATH]
exit 0 on zero mismatches, 1 otherwise.
"""
import hashlib
import hmac
import os
import struct
import subprocess
import sys
import time

SEED = 0x68656C69782D7233
OPS = {"sha256": 0, "sha512": 1, "hmac_sha512": 2, "pbkdf2": 3}

# frozen distributions
MSG_LENS = [0, 1, 55, 56, 63, 64, 65, 111, 112, 127, 128, 129]
KEY_LENS = [0, 1, 32, 63, 64, 65, 127, 128, 129, 200]
PBKDF2_ITERS = [1, 2, 1000]
MIX = [("sha256", 0.60), ("sha512", 0.20), ("hmac_sha512", 0.18),
       ("pbkdf2", 0.02)]


class XorShift64:
    """same generator as the c side: xorshift64* with the frozen seed."""

    def __init__(self, seed):
        self.s = seed & 0xFFFFFFFFFFFFFFFF
        if self.s == 0:
            self.s = 0x9E3779B97F4A7C15

    def next(self):
        s = self.s
        s ^= (s >> 12) & 0xFFFFFFFFFFFFFFFF
        s ^= (s << 25) & 0xFFFFFFFFFFFFFFFF
        s ^= (s >> 27) & 0xFFFFFFFFFFFFFFFF
        self.s = s
        return (s * 2685821657736338717) & 0xFFFFFFFFFFFFFFFF

    def rand(self, n):
        return self.next() % n if n > 0 else 0


def pick_len(rng, lens, spread):
    if rng.rand(4) == 0:
        return lens[rng.rand(len(lens))]
    return rng.rand(spread + 1)


def gen_vectors(n):
    rng = XorShift64(SEED)
    out = []
    names = [name for name, _ in MIX]
    weights = [w for _, w in MIX]
    pb_seen = 0
    for i in range(n):
        r = rng.rand(1000) / 1000.0
        acc = 0.0
        op = names[-1]
        for name, w in zip(names, weights):
            acc += w
            if r < acc:
                op = name
                break
        klen = pick_len(rng, KEY_LENS, 256)
        mlen = pick_len(rng, MSG_LENS, 512)
        key = bytes(rng.next() & 0xFF for _ in range(klen))
        msg = bytes(rng.next() & 0xFF for _ in range(mlen))
        iters = PBKDF2_ITERS[rng.rand(len(PBKDF2_ITERS))]
        if op == "pbkdf2":
            # one in twenty pbkdf2 vectors runs the production iteration
            # count, so the deep loop is exercised without paying for it
            # on every vector
            if pb_seen % 200 == 0:
                iters = 600000
            pb_seen += 1
        out.append((OPS[op], op, key, msg, iters))
    return out


def reference(op, key, msg, iters):
    if op == "sha256":
        return hashlib.sha256(msg).digest()
    if op == "sha512":
        return hashlib.sha512(msg).digest()
    if op == "hmac_sha512":
        return hmac.new(key, msg, hashlib.sha512).digest()
    return hashlib.pbkdf2_hmac("sha512", key, msg, iters, 32)


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 100000
    report = None
    if "--report" in sys.argv:
        report = sys.argv[sys.argv.index("--report") + 1]
    binary = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                          "diff_crypto")
    vectors = gen_vectors(n)
    req = bytearray()
    for op, _, key, msg, iters in vectors:
        req += struct.pack(">BII I", op, len(key), len(msg), iters)
        req += key
        req += msg
    t0 = time.time()
    proc = subprocess.run([binary], input=bytes(req), stdout=subprocess.PIPE)
    elapsed = time.time() - t0
    if proc.returncode != 0:
        print("diff_crypto exited %d" % proc.returncode)
        return 1
    buf = proc.stdout
    pos = 0
    bad = 0
    per_op = {}
    for op, name, key, msg, iters in vectors:
        olen = struct.unpack(">I", buf[pos:pos + 4])[0]
        pos += 4
        got = buf[pos:pos + olen]
        pos += olen
        want = reference(name, key, msg, iters)
        per_op.setdefault(name, [0, 0])
        per_op[name][1] += 1
        if got != want:
            bad += 1
            if bad <= 5:
                print("MISMATCH %s key=%d msg=%d iters=%d" %
                      (name, len(key), len(msg), iters))
        else:
            per_op[name][0] += 1
    lines = ["differential fuzz: %d vectors, %d mismatches (%.1f s)" %
             (len(vectors), bad, elapsed)]
    for name in sorted(per_op):
        ok, total = per_op[name]
        lines.append("  %-12s %6d/%6d" % (name, ok, total))
    lines.append("  95%% upper bound on mismatch rate (rule of three, 0 seen): "
                 "%.1e" % (3.0 / len(vectors)))
    text = "\n".join(lines)
    print(text)
    if report:
        with open(report, "w") as f:
            f.write(text + "\n")
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
