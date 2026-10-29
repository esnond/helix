# AGENT_LOG — r3-r5-run

Sequential agent runs, one per phase step. Order: methodologist -> architect
-> adversary -> memory -> editor. A crash is recorded and the item continues
without the agent; no phase stalls on an agent.

<!-- A1 -->
## A1 methodologist — 2026-10-29 — hypotheses review (phase 1, pre-run)

Verdict: `NOT VALID` as originally stated. Revision required before running.

- h1.1: registrable after revision. `sqrtf` is the only IEEE-754-pinned
  function; expf/sinf/cosf have no mandated bit pattern, so the "bits-exact
  per IEEE-754" wording was false. Missing symbols: `logf` and `sincosf`
  (GCC fuses sinf/cosf). Gate must be `nm -u` on the final binary, not
  "remove -lm from the key path" (the flag is global). Found an unreported
  boundary: `sscanf("%f")` in `vendor/petri/connectome.c:359` is libc/locale
  dependent and `herm_302.csv` is not hashed in the manifest.
- h1.2: `INVALID` — was a deliverable, not a testable claim, and the
  reduction target was wrong. A leaked `g_n` does not expose
  `sha256(mk||n)`; the correct assumption is PRF security of the keyed mask.
  12-word mnemonics carry 128 bits, not 256; keychain stores past
  generations in clear and must be excluded from the game.
- h1.3: registrable after minor revision (pin seed/generator, add HMAC
  key-length edge cases, label as smoke test).
- h1.4: registrable after revision (positive control too weak at d=0.025 to
  validate the harness; scope must state timing-only, cache out of scope).

Action taken: `docs/hypotheses.md` rewritten to carry the revised h1.1-h1.4
with explicit thresholds and disproof experiments.

Files: `docs/hypotheses.md`.

<!-- A2 -->
## A2 inline findings during h1.1 (adversary review deferred to phase gate)

Recorded here because they were found while implementing, not by an agent
run (agents are sequential; the adversary pass runs before the phase-1 gate).

- **DoS on unauthenticated generation field.** `dna_decrypt_file` parsed the
  header generation and immediately recomputed the chain before the tag was
  checked. Flipping bit 0 of header byte 41 turns generation 5 into
  16777221, and the recompute loop runs 16.7M sha256 recombinations in the
  test suite. Fix: `DNA_GEN_MAX` (4096) checked before the chain, return -2.
  Test updated: the old "header tamper" case relied on that slow path; it
  now flips a salt byte (true MAC failure, -4) and a separate case asserts
  the oversized generation is refused (-2).
- **Format bump.** Replacing libm changes key bits by design (h1.1), so
  `DNA_VERSION` is 2 and v1 files are refused with -2 instead of being
  decrypted with the wrong key. Pinned r2 key in test_kdf kept in a
  comment for the record.
- **`sscanf("%f")` on the connectome table** was a second unpinned boundary
  (locale + strtod). Replaced with a locale-free decimal parser; the table
  hash is now pinned in `tests/test_vectors.c`.

<!-- A3 -->
## A3 tooling results (no agent run; evidence for the phase-1 gate)

- h1.3 `tools/fuzz_crypto.py` + `tools/diff_crypto.c`: 0/100000 mismatch,
  upper bound 3.0e-5 (`results/fuzz_report.txt`).
- h1.4 `tools/ct_test.c`: keycheck t=0.128, mask t=-0.487, a/a 0.492,
  positive control t=1809.7 — harness detects a 10x branch, targets stay
  under 4.5 (`results/consttime/ct_test.txt`). Scope: timing only.
- First ct_test run had a broken class assignment (all traces in one
  class, t reported 0.000). Kept in the log because a silent t=0 in a
  constant-time report is exactly the failure mode to distrust.

<!-- A4 -->
## A4 findings during r3 (inline, before the phase gate)

- **All-zero salt in every v1/v2 file.** `dna_encrypt_file` filled the
  salt, then `memset(header, 0, ...)` wiped it before writing the magic.
  Verified on a generated file: bytes 5..36 were zero. Fixed by clearing
  first, filling after. New files carry a real 32-byte salt.
- **Test buffer overread.** `tests/test_dna.c` copied the whole file into a
  512-byte stack buffer; with the 1216-byte v2 header that wrote past it
  and segfaulted. The corrupt-file buffer is now heap sized.
- **Kyber cannot raise the mnemonic ceiling.** The keypair is derived from
  the generation key, so a quantum adversary holding the mnemonic gains
  nothing. Recorded in docs/r3_spec.md; the release text must not say
  "quantum-safe".
