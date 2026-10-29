# release v1.0 — gui closure + crypto closure

branch `v10-run`. seven stages, each with green gates (make test,
make test-san, make gate-libm, cargo test) and a checkpoint in
docs/STATE.md. spec: docs/r6_spec.md.

## shipped since v1.3

- gui rewritten from scratch per the design reference: mouse-first,
  both polarities, full ru/en dictionary, job queue with honest
  cancel, procedural sound engine (off by default, `--features audio`
  once libasound2-dev is installed). acceptance checklist:
  docs/gui_acceptance.md (owner signs).
- secure scrubbing: volatile memzero + mlock across the c core.
- .dna parser fuzzer: 20M + 1M asan/ubsan iterations, zero findings.
- .dna v3: external kyber-768 pair in a passphrase-encrypted .hkpair;
  session = sha256(kyber_ss || chain_key || tag). the mnemonic alone
  cannot decrypt v3. argon2id (rfc 9106 kat) for pair files; encrypt
  refuses to overwrite an existing output.
- keychain kch2: aead over the whole chain, fresh nonce per rewrite,
  flock, kch1 migrates in place (auto on first evolve or
  `helix migrate-kch`).
- musl and arm64 (qemu) replays: identical keys, v2/v3 roundtrips,
  cross decrypt.

## evidence index

- hypotheses h4.1-h4.5: docs/hypotheses.md (kat bytes, refusals,
  sanitizer logs)
- format: docs/dna_format.md (v3 section), keychain header in
  src/keychain.c
- cross platform: results/cross_platform/libc_*.log, arch_*.log
- fuzzer: results/fuzz_dna/run_20261029T*.log
- reviews: docs/AGENT_LOG.md A6-A10 (inline; spawns blocked)

## owner acceptance (blocks the v1.0 tag)

docs/gui_acceptance.md — 29 checks, hands on. after the checklist
passes, tag v1.0 and push with scripts/push_run.sh (owner types the
pat at git's own prompt).

## known residuals

docs/limitations.md items 11-13 (pair file loss is permanent, cli -p
in shell history, decrypt overwrites), plus items 1-10 (unaudited
crypto, timing-only side channels, pbkdf2 derivation, urandom).
