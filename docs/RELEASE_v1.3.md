# helix v1.3 — release notes

Date: 2026-10-29. Tag: `v1.3`. Base: r2 (`66b3485` lineage), branch
`r3-r5-run`, anchor tag `r2-anchor`.

**Still a research prototype.** No audit, no production use. The honest
one-liner: the trajectory does not add entropy, it adds per-generation
diversity and cost.

## What this release closes

Four criticisms were open at r2. Each now has evidence, not an assertion:

| criticism | closure | evidence |
|---|---|---|
| expf/sqrtf/sinf/cosf on the key path are not pinned across libc | the path carries its own math; no libm symbol in the binary; key identical at -O0..-O3; connectome table hashed | `make gate-libm`, `results/float_math/`, `docs/hypotheses.md` h1.1 |
| no argument that a leaked generation does not weaken the key | prf-game reduction with an explicit loss term, keychain exclusion and the 128-bit ceiling | `docs/security_proof.md`, h1.2 |
| crypto from scratch needs evidence beyond vectors | 100000 differential vectors vs python hashlib/hmac, 0 mismatches; timing harness with a positive control | `results/fuzz_report.txt`, `results/consttime/`, h1.3 h1.4 |
| cross-platform claim scoped to one machine class | libm part closed; architecture/libc part still open and labelled as an assumption with a ready script | `docs/limitations.md` 4, `scripts/cross_libc.sh` |

## Features

- **r3 — kyber-768 hybrid, .dna v2.** `kdf_type=1` wraps the session key
  with `sha256(chain_key || kyber_ss || tag)`. The keypair is derived from
  the generation key so the mnemonic alone still decrypts. This is
  plumbing; it does **not** raise the ceiling for an attacker with the
  mnemonic (`docs/r3_spec.md`).
- **r5 — gui in the v5 design language.** dual polarity, 340 px keys
  column, hairlines, jobs strip, operation journal, dendrite chain, RU/EN.
  Deferred and named in `docs/r5_spec.md`: sound engine, dialog morph,
  nacre sweep, dialog-body i18n.
- **r4' — cancelled.** No helixd daemon. `docs/r4_decision.md` has the
  reasoning and the falsifier that would reopen it.

## Breaking changes

- **.dna v1 files are refused** (`-2`), not mis-decrypted. Two reasons:
  the vendored math changed the key bits by design, and the old header
  carried an all-zero salt. Migration: `git checkout r2-anchor && make`,
  decrypt there.
- `dna.h` API unchanged; `DNA_HEADER_SIZE` is now 1216.

## Bugs found and fixed while doing the above

- every file written before this release had an all-zero salt (the salt
  was written, then wiped by a `memset`); fixed, and a generated file is
  checked by the test suite.
- an unauthenticated generation field drove up to 2^32 sha256
  recombinations before authentication; now bounded by `DNA_GEN_MAX`.
- `sscanf("%f")` on the connectome table was a second unpinned boundary;
  replaced with a locale-free parser and pinned by hash.
- a 512-byte stack buffer in the dna test overflowed once the header grew;
  the corrupt-file buffer is heap sized now.

## Verification

```
make test        # 11 c test binaries + shell gates, all green
make test-san    # asan + ubsan, all green
make gate-libm   # no libm symbol in the shipped binary
cargo test       # gui: 2 tests, both drive the c core through ffi
```

Numbers: 100000/100000 differential vectors match; keycheck timing
t=0.128 (2.0m traces), mask t=-0.487 (1.0m), positive control 1809;
derivation 3.4 s at 600k pbkdf2 iterations and 10000 engine steps.
