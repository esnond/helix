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

<!-- A5 -->
## A5 r4-prime decision, recorded without an agent run

Both options were weighed in docs/r4_decision.md: a resident Erlang/OTP
batch daemon versus documented cancellation. Cancellation wins on four
grounds — no consumer after HX-7, attack surface, the seam rule (there is
no simulator artefact to hand over), and the absence of a supervision test
harness. A falsifier is written down so the decision can be reopened on
evidence rather than on mood.

<!-- A6 -->
## A6 v1.0 run, stage 1 kickoff

architect spawn blocked provider-side ("unusual activity"), same class as
the 2026-10-28 outage. Per R1 the stage continues without the agent; the
skeleton design is done inline and recorded here:

- files: main, app, prefs, theme, fonts, i18n, jobs, panel_files,
  panel_keys, panel_jobs, bar_status, dialog, sound/{mod,engine,sources};
  core/helix_wrapper.rs and gui_api.c survive unchanged (the one unsafe
  boundary stays the FFI shim).
- jobs: one worker thread per job, cancel = flag + drop of a late result
  (the C call is uninterruptible, ~3.4 s); events via mpsc; queue shows
  reason, indeterminate hairline, cancel.
- sound: engine thread owns the cpal stream, commands over mpsc into a
  render ring; procedural metal-box IR, wet 0.13, one-pole lowpass 14k,
  soft compressor; 6 sources per constitution §8; cpal init failure
  degrades to a silent backend with a visible "audio unavailable" note.
- dialog state machine: Idle -> Opened{Seed|Encrypt|Decrypt|Delete} ->
  Busy -> Closed, error and glitch ride along.

<!-- A7 -->
## A7 methodologist pass, inline (spawn blocked again, provider-side)

Verdicts against head b320457:
1. every-action-by-mouse: SUPPORTED — encrypt/decrypt/delete/up/refresh/
   phrase/evolve/reveal/polarity/lang/sound all have buttons; j/k/e/h are
   accelerators only.
2. no-crypto-in-rust: SUPPORTED — `grep -rn unsafe helix-gui/src` hits only
   core/helix_wrapper.rs; no rust file derives key material.
3. sound-default-off: SUPPORTED — prefs::default sound=false; the toggle
   writes the conf file.
4. cancel: PARTIAL — the cancel flag is checked after the c call returns
   (jobs.rs run_op), so a cancelled encrypt/decrypt still writes its output
   file; only the row label and the UI event are dropped. Recorded in the
   acceptance checklist item 18. Real interruption needs a core-side
   progress callback — deferred to the v3 stage (header write happens last,
   so a cancelled decrypt leaves at most a complete plaintext copy the user
   explicitly asked to cancel: named residual risk).
5. checklist gaps: none found beyond item 18 wording, now fixed.

required fixes: none blocking stage 1 exit; cancel semantics revisited in
stage 4 when the job model gains the keypair path.

<!-- A8 -->
## A8 adversary pass, inline (spawn blocked, provider-side, third time)

Findings on the rewritten gui:
- P2 overwrite: encrypting "x" writes "x.dna" via fopen "wb" — an existing
  archive with that name is replaced silently. Mitigation (stage 4, with
  the v3 work): dna_encrypt_file refuses when the output exists unless
  --overwrite / dialog confirm. Filed in STATE.
- P3 symlink: remove_file on a scanned path removes the link, not the
  target — no privilege gain; no shell interpolation anywhere (no Command),
  so crafted filenames cannot execute anything.
- P3 busy race: confirming a dialog while the queue is busy is refused with
  a journal line; the dialog closes and the user must retry. Acceptable.
- P3 phrase lifetime: the phrase lives in a rust String for the session
  (reveal dialog reads it). No zeroization on the rust side — same class as
  limitation 9 in the c core; named, not hidden.
No P0/P1. Stage 1 may exit.

## A9 — stage 4 review pass (inline; spawns still blocked)

methodologist: hypotheses h4.1–h4.5 registered with falsification
criteria, all five supported with recorded evidence (kat bytes,
refusal rc codes, sanitizer logs). no claim in dna_format.md goes
beyond the tests; the "not decrypt-anywhere" property is stated
plainly.

adversary:
- P2 pair file is the new root secret: .hkpair brute force is bounded
  by argon2id (64 MiB) or pbkdf2 600k; passphrase entropy is the
  owner's. accepted residual, documented in dna_format.md.
- P3 cli -p passphrase is visible in shell history/process list; gui
  uses a password field. documented for the release notes.
- P3 losing the .hkpair loses the v3 archives permanently; the gui
  pair section shows the path so the file gets backed up. release
  notes item.
- P3 gui pair export writes <pair>.pub, overwriting an existing .pub
  silently; same class as the fixed encrypt overwrite, low impact
  (public material). named, not fixed.
- checked: encaps uses fresh os entropy per file (no seed reuse),
  v3-in-v2 header rejected, hkpair_load bounded reads (-2 on short
  file), pair create refuses overwrite, iterations field aad-bound.
No P0/P1. Stage 4 may exit.
