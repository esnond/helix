# threat model

scope: helix v1.0, as an offline cli tool plus a local gui. written
for r1, updated as the scope grew (r2 .dna, r3 kyber, v1.0 v3 pair +
kch2). status: research prototype (see the README warning).

## assets

1. the mnemonic (master seed, 12-24 bip39 words) - root of everything
2. the .hkpair pair file + its passphrase - second factor of every
   v3 archive (v1.0); losing it loses those archives permanently
3. master key material in process memory during derivation
4. generations in the keychain: kch1 stored them in plaintext; since
   kch2 (v1.0) the entry area is one chacha20poly1305 record under
   sha256(mk || "helix-kch") — a disk thief without the mnemonic gets
   nothing, and any modification is refused on open
5. files encrypted with generation keys (.dna v1/v2/v3)

## attacker models

### passive observer of ciphertexts (r2 scope)

sees .dna files only. gets: generation number, salt, headers. cannot
decrypt without the corresponding key. mac detects modification (r2).

### local attacker with disk access

r1 verdict (kch1): the keychain gave every past generation in
plaintext; mitigation was os level only (0600). that gap is closed in
v1.0: kch2 encrypts the whole entry area (docs/r1_security.md keeps
the original r1 analysis verbatim as history). what the disk thief
still gets: the mnemonic if it is stored somewhere, the .hkpair
passphrase if it was typed in a shell (limitations 12), and the .dna
ciphertexts. future keys stay safe without the mnemonic (masks are
sha256(mk, n), preimage ~2^256).

### local malware / same user

can read process memory, keychain, mnemonic as typed. nothing in
helix's threat model defends against this (defending requires
os support: memlock, keyring, sgx - out of scope for a prototype).

### co resident attacker (timing / cache)

measures execution timing or cache state of helix processes.
status after the hardening pass: no exploitable oracle found.
keycheck is offline (caller sees the result), the mutation selection
is branchless with fixed trip count, sha/hmac cores branch free on
secret data, memory access line constant. bip39 wordlist binary
search branches on the secret mnemonic - accepted for offline use
(richer local attacks exist at this threat level), revisit at r4.
measurements: results/timing_analysis/, results/side_channel_audit/.

### network attacker (r4 scope, cancelled)

the r4 directory service and helixd daemon were cancelled on the
record (docs/r4_decision.md); nothing listens on a socket. the timing
masking requirement stands as a precondition for any future network
exposure.

### quantum attacker

grover halves symmetric security: pbkdf2 cost drops, sha256 mask
preimage ~2^128 - the design degrades, does not collapse. r3 shipped
a kyber-768 hybrid for .dna v2, and v1.0's v3 format encapsulates to
an external kyber-768 pair — but kyber-768 is a pre-quantum
assumption with no audit, and v2's pair still derives from the
mnemonic, so "quantum resistant" is not a claim helix makes
(docs/limitations.md 1, README).

## what helix protects against (r1, offline)

- brute forcing the mnemonic: pbkdf2 600k + engine trajectory per
  guess (seconds per guess)
- key reuse: every generation is a distinct 256 bit value
- predicting future generations from the keychain without the
  mnemonic
- silent keychain corruption: header/fingerprint validation, torn
  tail repair

## what helix does not protect against

- compromise of the mnemonic (total loss, by design)
- keychain theft by a mnemonic holder (kch2 hides it from a disk
  thief without the mnemonic, not from the owner's own adversary)
- malware on the host
- side channels beyond those measured
- cross environment replay errors (libm and musl/arm64 boundaries
  closed in v1.0, results/cross_platform/; macos/windows unproven)

## out of scope

multi user operation, hardware side channels, formal verification of
the engine, denial of service. concurrent keychain writers on one
file are serialized by flock since kch2 (second process gets -6).
