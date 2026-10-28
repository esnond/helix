# roadmap status after r2 (2026-10-28)

## done

- r0: seed generation + deterministic key derivation (pinned value)
- r1: key evolution, keychain, forward secrecy analysis
- security hardening: timing, side channels, five config matrix,
  honest repositioning, threat model
- r2: .dna encrypted archive format (docs/dna_format.md), chacha20
  poly1305 per rfc 8439 with verbatim rfc vectors, cli encrypt/
  decrypt, 100 mb in 3.6 s. suite: 10 test binaries + shell gates,
  all green.

## r3: post quantum hybrid kem — next, not started

plan (brief option C): vendor the pq-crystals/kyber reference c
implementation under vendor/pqcrystals-kyber with full provenance
notes (unaudited reference code, same honesty rules as the petri
vendor import), wrap in src/kyber_wrapper.c, add kdf_type 1 to the
.dna header (backward compatible, old files stay kdf_type 0),
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

estimate: a full session (vendoring, kat wiring, hybrid, tests).

## r4: directory service — blocked on toolchain

erlang is not installed on this machine (no erl, no rebar3). no
skeleton is committed: code that cannot compile or run its tests does
not land (green gate). unblock: sudo apt install erlang-base
erlang-dev (and rebar3 if desired).

design carried over from the brief, with hardening-pass amendments:

- challenge auth = hmac over a server nonce with the generation key;
  the h1 timing analysis requirement applies: a network verifier must
  add uniform response delay or batch responses before exposure
  (results/timing_analysis/mitigation_applied.md)
- the keychain watcher (evolve --publish) must be read only toward
  the keychain file; concurrent writers are out of scope per r1

## r5: gui — blocked on toolchain and display

no rust toolchain (no cargo, no rustc) and no wayland/x11 session
available for validation here. same rule: nothing lands that cannot
be built and tested. unblock: rustup + a wayland or x11 session;
then winit/egui front end over the existing cli or a thin ffi.

## sequencing note

r3 is pure c and fits this repo and this toolchain: it is the only
remaining milestone with no environment blocker. r4 and r5 wait for
the environment, not for design work.
