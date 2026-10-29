# r6 spec — v1.0 (gui rewrite + crypto closure)

v1.0 closes the gap between the shipped project and its design
reference (docs/design/constitution.md, v5_reference.html) and the
crypto holes named in the v1.3 review. stages, all with green gates
(make test, make test-san, make gate-libm, cargo test) and state
checkpoints in docs/STATE.md:

## s1 — gui rewrite from scratch

eframe/egui 0.23 over the same c core through gui_api.c (the only
unsafe boundary). mouse-first: click selects, double click opens,
labelled buttons for every action, keyboard only as accelerators.
modules: app, panel_files, panel_keys, panel_jobs, bar_status,
dialog (one modal), theme (both polarities), sound/{engine,sources},
i18n (full ru/en dictionary, parity tested), prefs (persisted).
sound per constitution §8: procedural sources, metal-box convolver,
compressor, lowpass; off by default; cpal behind the `audio` cargo
feature. cancel semantics honest: the running c call finishes
(~3.4 s), the late result is dropped. evidence: cargo test 7/7,
docs/gui_acceptance.md checklist.

## s2 — secure scrubbing

src/secure_mem: volatile-pointer memzero the compiler cannot elide,
mlock/munlock page pinning (degrades loudly). applied across the
crypto core (kdf, evolve, keycheck, aead, chacha, poly1305, hmac,
pbkdf2, sha, bip39, dna, kyber_wrap). residual: rust-side strings
hold the phrase; named in limitations.

## s3 — .dna parser fuzzer

tools/fuzz_dna: deterministic mutation (xorshift64*, 1-8 mutations)
over a real 1216-byte corpus, asan+ubsan. 20,000,000 iterations, 0
memory findings, 261321 expected rejections
(results/fuzz_dna/run_20261029T172553Z.log). rerun after the v3
parser change: 1,000,000 iterations, 0 findings
(results/fuzz_dna/run_20261029T192833Z.log).

## s4 — .dna v3: external kyber pair + argon2id

kdf_type=2: session = sha256(external kyber ss || chain_key ||
"helix-v3-external"). the pair lives in a passphrase-encrypted .hkpair
(chacha20poly1305; pbkdf2-600k or argon2id t=3 m=64MiB p=4). the
mnemonic alone no longer decrypts v3 — two factors. argon2id is
implemented per rfc 9106 and pinned to the section 5.3 kat
(test_argon2id 4/4); blake2b per rfc 7693. encrypt refuses to
overwrite an existing output (a8 p2, error -6). cli: pair
create/export, migrate-kch; encrypt/decrypt -P pair -p passphrase.
format: docs/dna_format.md v3 section. tests: test_hkpair 25/25.

## s5 — keychain kch2

the whole entry area is one aead record under sha256(mk ||
"helix-kch") with a per-rewrite nonce counter; the header binds
current_gen, counter, length and body offset. crash safe: body first,
header flip last, orphan tails truncated on open. flock exclusive
access (-6 busy). legacy kch1 opens read-only and migrates in place
on the first append or via `helix migrate-kch`. tests: test_keychain
64/64.

## s6 — cross platform

scripts/cross_libc.sh: musl vs glibc keys identical.
scripts/cross_arch.sh: aarch64 (-static) under qemu-aarch64 — derive
key identical to x86-64, v2 and v3 roundtrips, x86-64 decrypts arm64
output, aarch64 test_dna passes. evidence: results/cross_platform/.
no release blocker.

## out of scope, named

streaming .dna (files > ~100 mb buffer whole), key rotation for lost
generation material, macos/windows replays, third-party audit, the
audio feature build on machines without libasound2-dev (silent
backend by design), gui export overwriting an existing .pub (p3,
named in AGENT_LOG A9).
