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
