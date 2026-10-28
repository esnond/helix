# helix

> **WARNING: research prototype, NOT production cryptography.**
> do not use this for real secrets. the crypto is written from
> scratch and has never been audited, the key derivation rests on a
> float based simulation whose bitwise replay is proven only within
> one build environment class, and the side channel surface is
> analyzed but not eliminated. for production use established
> libraries: libsodium, age, openssl. this project exists to study a
> question, not to protect anything.

research prototype: biological dynamics in a key derivation function.
a bip39 seed drives a petri connectome simulation, and the trajectory
of the network is the key. the honest one line summary: the
trajectory provably does not add entropy - every key is a deterministic
function of the master key - what the simulation adds is per
generation key diversity and computational cost, wrapped in a fully
reproducible pipeline.

c99, posix subset, no external libraries. the petri engine (c.
elegans connectome simulator, v1.0.1) is vendored verbatim under
vendor/petri, see vendor/petri/VENDORED.md for provenance and hashes.

status: r1 + security hardening pass. seed generation, deterministic
key derivation (r0) and key evolution (r1) work: an append-only
keychain, xor recombination with bounded bit mutation, generation
retrieval, shell-level chain determinism. timing analysis, side
channel audit and a five configuration determinism matrix are in
results/. the .dna archive format (r2), post quantum crypto (r3),
the directory service (r4) and the gui (r5) are future milestones.
r1 details: docs/r1_spec.md, docs/r1_security.md.

## what this is

- research on using biological dynamics (a 448 cell connectome
  trajectory) as a key derivation stage
- proof of concept that the whole pipeline is bitwise deterministic
  within a build environment class (five compiler configurations,
  one key)
- an educational, fully from scratch implementation of sha256/sha512/
  hmac/pbkdf2/bip39 with every value pinned to stdlib cross checks

## what this is not

- an audited cryptographic library
- a production key manager
- a replacement for argon2, scrypt, pbkdf2-as-deployed, age or
  libsodium
- quantum resistant (r3 planned, nothing shipped)

## security guarantees

guaranteed (test enforced):

- same mnemonic + salt + build environment = same key, bitwise
  (pinned in tests, five config matrix in results/cross_platform/)
- bip39 compliance with the official vector set (24/24, passphrase
  TREZOR)
- constant time comparison in keycheck, branchless mutation
  selection (results/timing_analysis/, results/side_channel_audit/)

not guaranteed:

- side channel resistance beyond what is measured (power, em,
  microarchitectural attacks are unmeasured)
- bitwise replay across architectures, operating systems or libm
  versions (float analysis: results/cross_platform/float_analysis.md)
- any strength claim for the connectome stage beyond computational
  cost (no formal security proof exists or is claimed)

needs further research: a reduction argument for the trajectory
stage, cross platform replay, third party audit. see
docs/threat_model.md, docs/security_claims.md, docs/limitations.md.

## build

    make            # -std=c99 -pedantic -Wall -Wextra -Werror -O2
                    # -ffp-contract=off, warnings are errors
    make test       # unit vectors, kdf determinism, shell cmp gate
    make test-san   # whole suite under asan+ubsan (~90 s)
    make clean

## usage

generate a seed (24 words default, 12 with -w 12):

    ./helix gen-seed
    ./helix gen-seed -w 12 -e     # also print the raw entropy

derive a key (stateless gen 0, r0 semantics):

    ./helix derive -m "word1 word2 ..." [-s salt_hex] [-c connectome.csv]

key evolution (r1): the chain is a pure function of the mnemonic, the
keychain is a cache. gen 0 is the stateless derivation, each further
generation is xor recombination of the two parents plus a bounded
mutation of 32..64 bits (see docs/r1_spec.md):

    ./helix evolve -m "..."        # first call bootstraps gens 0 and 1
    ./helix evolve -m "..."        # advances to gen 2, 3, ...
    ./helix derive -m "..." -g 5   # fetch generation 5 from the keychain

keychain: append-only $HOME/.helix-kch-<fingerprint>.db, 0600, all
past generations retrievable. evolution stats: 32..64 flips per
generation (mean 48), consecutive-generation hamming distance mean
127.7 (results/r1_evolution_report.md).

key hex on stdout, engine stats on stderr. exit codes: 0 ok,
2 invalid mnemonic, 3 engine init failure, 4 keychain error.

## key derivation (r0 spec)

1. entropy = bip39 decode of the mnemonic, checksum verified
   (16 bytes for 12 words, 32 for 24)
2. master_key = pbkdf2_hmac_sha512(entropy, salt, 600000 iterations,
   64 bytes)
3. the vendored engine is initialized with connectome
   vendor/petri/data/herm_302.csv (448 cells: 302 neurons plus
   muscle/passive targets) and seeded from the master key:
   rng seed = master_key bytes 0..7 big endian; each cell's initial
   membrane potential is offset by master_key byte 8+(i mod 56)
   interpreted signed, scaled to +/- 2 mv
4. 10000 simulation steps at the engine dt of 1 ms
5. after every step, every cell's activation (sigmoid transfer,
   0..1) is quantized to one byte (round to nearest) and streamed
   into sha256
6. derived_key = sha256(trajectory || master_key), 32 bytes. the
   master key is folded into the digest because the trajectory is
   contractive under a fixed noise stream: membrane offsets alone
   would not keep master key bytes 8..63 load bearing

determinism: same mnemonic + salt + connectome + binary gives the
same 32 bytes, bitwise. enforced by the suite: in process replay
(test_kdf) and two cli runs compared with cmp (run_tests.sh). this
inherits the engine's determinism discipline: xorshift64* only, no
wall clock in the sim path, fixed fp operation order under
-ffp-contract=off.

salt: r0 accepts an arbitrary hex salt, default empty. the empty
default is a placeholder; the .dna format (r2) will carry a unique
per file salt. do not rely on the empty salt for anything real.

## security claims

r1 forward secrecy is analyzed honestly in docs/r1_security.md:
single leaked generation hides its neighbors behind ~2^256 masks, but
the keychain stores every past key in plaintext, so keychain
protection equals mnemonic protection. what r0/r1 do guarantee:
bitwise reproducibility, checksum validated seeds, and zeroization of
master key material after use (best effort, documented memset; the
simulator also holds derived state).

sha256, sha512 (fips 180-4), hmac-sha512 (rfc 2104), pbkdf2
(rfc 8018) written from spec, no external crypto library. round
constants are generated from first principles (integer sqrt/cbrt of
the first primes) by tools/gen_vectors.py, which also emits the
wordlist header and all expected test values. every primitive is
pinned against python hashlib/hmac stdlib output, rfc 4231 hmac
cases, and the official bip-0039 english vector set (24 vectors,
passphrase TREZOR): 175 checks green, plus asan+ubsan.

the english bip39 wordlist is pure ascii, so nfkd normalization is
the identity; passphrases with non ascii characters are not
normalized (documented in src/bip39.h).

## security claims, r0

known limitations, honest:

- cross platform replay is expected to hold where the engine's does
  (same architecture class, same flags) but is only tested on this
  machine; a key derived elsewhere must be verified against a
  recorded key fingerprint before trust
- 2.8 s per derivation is by design (brute force cost), but it also
  makes interactive use slow
- /dev/urandom only for seed generation; no hwrng fallback
- memory zeroization is best effort memset, not page locking

## layout

    src/          helix code: crypto core, bip39, kdf driver, cli
    vendor/petri/ verbatim engine import, provenance in VENDORED.md
    data/         bip39 english wordlist, official vector cache
    tests/        vector and determinism tests, run_tests.sh
    tools/        gen_vectors.py: generated headers + provenance

## connection to petri

the engine keeps its semantics untouched: helix seeds it and reads
activations, nothing inside vendor/ is modified. the derivation is
therefore exactly as deterministic as the engine's own published
determinism gates, and any future engine determinism regression
shows up here as a kdf determinism failure.
