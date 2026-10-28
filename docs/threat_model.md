# threat model

scope: helix r1 + security hardening pass, as an offline cli tool.
status: research prototype (see the README warning).

## assets

1. the mnemonic (master seed, 12-24 bip39 words) - root of everything
2. master key material in process memory during derivation
3. generations stored in the keychain file (all past keys, plaintext)
4. files encrypted with generation keys (r2, future)

## attacker models

### passive observer of ciphertexts (r2 scope)

sees .dna files only. gets: generation number, salt, headers. cannot
decrypt without the corresponding key. mac detects modification (r2).

### local attacker with disk access

reads keychain.db: obtains every past generation in plaintext.
mitigation is os level only (0600, user separation). does not obtain
the mnemonic or future generations (masks are sha256(mk, n),
preimage ~2^256). verdict: keychain compromise = past keys lost,
future keys safe while the mnemonic is safe. documented honestly in
docs/r1_security.md.

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

### network attacker (r4 scope, not yet applicable)

the directory service will expose generation verification over the
network. timing masking (uniform response or delayed batch) is a
recorded r4 requirement before any network exposure.

### quantum attacker

grover halves symmetric security: pbkdf2 cost drops, sha256 mask
preimage ~2^128 - the design degrades, does not collapse. shor is
irrelevant until public key crypto lands (r3 planned: kyber/
dilithium hybrid). nothing quantum resistant is shipped today.

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
- keychain theft (all past keys)
- malware on the host
- side channels beyond those measured
- cross environment replay errors (libm/arch differences - detect
  via fingerprint verification, see float_analysis.md)

## out of scope

multi user operation, concurrent keychain writers, hardware side
channels, formal verification of the engine, denial of service.
