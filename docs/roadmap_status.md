# roadmap status — v1.0 (2026-10-29)

## done

- r0: seed generation + deterministic key derivation (pinned value)
- r1: key evolution, keychain, forward secrecy analysis
- security hardening: timing, side channels, five config matrix,
  honest repositioning, threat model
- r2: .dna encrypted archive format (docs/dna_format.md), chacha20
  poly1305 per rfc 8439 with verbatim rfc vectors, cli encrypt/
  decrypt, 100 mb in 3.6 s. suite: 10 test binaries + shell gates,
  all green.
- r3: post quantum hybrid — vendor/kyber (pq-crystals reference,
  KYBER_K=3), .dna v2 with kdf_type 1, tests/test_kyber.c (14
  checks), docs/r3_spec.md. honest boundary stated there: the kyber
  keypair derives from the generation key, so v2 is hybrid plumbing,
  NOT quantum safety.
- r5: gui in the v5 design language (egui, both polarities, i18n).
- v1.0 (branch v10-run, tag v1.0-rc): gui rewritten from scratch,
  secure scrubbing, .dna parser fuzzer (20M asan clean), .dna v3
  external kyber pair + argon2id (rfc 9106 kat), keychain kch2
  (aead, flock, kch1 migration), musl + arm64 replays with identical
  keys. spec with evidence pointers: docs/r6_spec.md.

## r4: directory service — CANCELLED

cancelled on the record (docs/r4_decision.md): the erlang control
plane duplicates what the offline tool already does without a
network surface, and a network verifier would reopen the h1 timing
exposure. the design requirements (hmac challenge auth, uniform
response delay before any network exposure) stand as preconditions
if the idea is ever revisited.

## open items (value order)

1. third party audit of the whole c core (the standing item 1 in
   docs/limitations.md; nothing the project can do alone)
2. cache side channel measurement for the secret-derived indexing in
   src/evolve.c (timing is measured, cache is not)
3. streaming .dna mode for files beyond ram (~100 mb today)
4. derivation kdf stays pbkdf2-600k; argon2id exists for pair files
   only — switching the derivation forks every keychain, undecided
5. macos / windows / big-endian replays
6. gui: drag-and-drop, multi-select; sound feature needs
   libasound2-dev on this machine (silent backend otherwise)

## historical snapshot (r3-era, kept for the record)

the r3-era roadmap below described the state when rust, erlang and
musl were absent from the machine. all three blockers were cleared;
the plan-vs-shipped deltas are recorded in docs/r6_spec.md.

### r3 plan as written before the run

plan (brief option C): vendor the pq-crystals/kyber reference c
implementation with full provenance notes (unaudited reference code,
same honesty rules as the petri vendor import), wrap in a wrapper
module, add kdf_type 1 to the .dna header (backward compatible),
hybrid shared secret = sha256(kyber_ss || bio_ss).

integration notes recorded in advance:

- the reference implementation is not constant time; the security
  claims file must say so explicitly, hybrid or not
- vendor code must pass the same -std=c99 -wall -wextra -werror gate
  or carry a documented per file exception in the commit message
- the reference randombytes() hook is replaced by src/entropy.c
- kat vectors ship with the reference repo; they pin the import the
  same way the rfc vectors pin chacha20
- adversary review before the format gains kdf_type 1 in the wild

all five notes were honored; see docs/r3_spec.md and
docs/AGENT_LOG.md.
