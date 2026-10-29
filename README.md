# helix

> **WARNING: research prototype, NOT production cryptography.**
> do not use this for real secrets. the crypto is written from
> scratch and has never been audited, and the side channel surface is
> analyzed for timing only (cache, power, em unmeasured). for
> production use established libraries: libsodium, age, openssl. this
> project exists to study a question, not to protect anything.

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

status: v1.0. seed generation, deterministic key derivation (r0),
key evolution (r1), the .dna archive format (r2), a kyber-768 hybrid
.dna v2 (r3), a gui in the v5 design language (r5) and the v1.0
closure are done. v1.0 adds: the .dna v3 two-factor format (external
kyber-768 pair in a passphrase-encrypted .hkpair + argon2id per rfc
9106, kat-pinned), the encrypted keychain kch2 (aead, flock, kch1
in-place migration), secure secret scrubbing (volatile memzero +
mlock), a 20M-iteration asan/ubsan parser fuzzer, a rewritten
mouse-first gui with procedural sound, and musl + arm64 replays with
identical keys (results/cross_platform/). earlier closures: libm
boundary gone (h1.1), prf-game reduction (h1.2, docs/security_proof.md),
100k differential vectors + constant-time harness (h1.3, h1.4). the
r4 directory service and helixd daemon are cancelled on the record
(docs/r4_decision.md). milestone docs: docs/r1_spec.md,
docs/r1_security.md, docs/dna_format.md, docs/r3_spec.md,
docs/r5_spec.md, docs/r6_spec.md, docs/security_proof.md.

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
- quantum resistant (kyber-768 is a pre-quantum assumption; v3 adds
  an external pair factor, not a quantum proof)

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
- bitwise replay on macos, windows or big-endian targets (musl and
  arm64 replays are closed: results/cross_platform/)
- any strength claim for the connectome stage beyond computational
  cost (no formal security proof exists or is claimed)

needs further research: a reduction argument for the trajectory
stage, cross platform replay, third party audit. see
docs/threat_model.md, docs/security_claims.md, docs/limitations.md.

## build

    make            # -std=c99 -pedantic -Wall -Wextra -Werror -O2
                    # -ffp-contract=off, warnings are errors
    make test       # 14 suites: kats, determinism, fuzz-checked format
    make test-san   # whole suite under asan+ubsan
    make gate-libm  # no libm symbol on the derivation path
    make clean
    cd helix-gui && cargo build --release   # gui (see below)

## gui

    cd helix-gui && cargo build --release
    install -Dm755 target/release/helix-gui ~/.local/bin/helix-gui

mouse-first file manager over the same core: click to select, double
click to open, labelled buttons for every action; keys column with
the evolution chain, external pair section (create / export pk), job
queue with cancel, ru/en interface, both polarities, settings
persisted in ~/.config/helix/gui.conf. procedural sound (tick, plate,
line, hum, fault per the constitution) behind the `audio` cargo
feature — build with `cargo build --release --features audio` once
libasound2-dev is installed; without it the gui runs silent and says
so in the status bar. acceptance checklist: docs/gui_acceptance.md.

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

encrypted archives (.dna format, r2/v2 and v3):

    ./helix encrypt report.pdf report.pdf.dna -m "..."
    ./helix decrypt report.pdf.dna report.pdf -m "..."

v2 decryption needs only the mnemonic; the generation key is
recomputed from the evolution chain, the keychain is a cache. v3
files need a second factor — an external kyber-768 pair in a
passphrase-encrypted .hkpair file; the mnemonic alone returns an
authentication error:

    ./helix pair create pair.hkpair -p "pair passphrase" [-k pbkdf2|argon2id]
    ./helix pair export pair.hkpair pair.pub -p "pair passphrase"
    ./helix encrypt f.bin f.bin.dna -m "..." -P pair.hkpair -p "pair passphrase"
    ./helix decrypt f.bin.dna f.bin -m "..." -P pair.hkpair -p "pair passphrase"

encrypt refuses to overwrite an existing output (exit 6). the pair
kdf defaults to argon2id (rfc 9106 profile, kat-pinned; -k pbkdf2 for
the 600k pbkdf2 profile). losing the .hkpair loses those archives
permanently — back it up. format spec: docs/dna_format.md (v2 and v3
sections).

keychain kch2: $HOME/.helix-kch-<fingerprint>.db, 0600. the whole
entry area is one chacha20poly1305 record under sha256(mk ||
"helix-kch") with a fresh nonce per rewrite, and an exclusive flock
guards the file while a handle is open. legacy kch1 files are
migrated in place on the first evolve, or explicitly:

    ./helix migrate-kch -m "..."

evolution stats: 32..64 flips per generation (mean 48),
consecutive-generation hamming distance mean 127.7
(results/r1_evolution_report.md).

key hex on stdout, engine stats on stderr. exit codes: 0 ok,
1 io, 2 format, 4 key/generation unavailable, 5 authentication,
6 output exists.

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
single leaked generation hides its neighbors behind ~2^256 masks.
since kch2 the keychain itself is ciphertext without the mnemonic,
but whoever holds the mnemonic can recompute every generation — the
master key remains the root of trust. zeroization uses a volatile
memzero the compiler cannot elide plus mlock page pinning
(src/secure_mem.c, tests/test_secure_mem.c); the rust gui still holds
the phrase in plain strings, documented in docs/limitations.md.

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

## known limitations, honest

- the crypto core is unaudited; hand written primitives are a red
  flag by default (docs/limitations.md item 1)
- side channels: timing measured with a calibrated harness; cache,
  power and em are unmeasured
- 2.8 s per derivation is by design (brute force cost), but it also
  makes interactive use slow
- /dev/urandom only for seed generation and v3 encapsulation; no
  hwrng fallback
- a v3 archive without its .hkpair file is unrecoverable
  (docs/limitations.md item 11)

the full list with residuals lives in docs/limitations.md; this
section does not duplicate it.

## layout

    src/            helix code: crypto core, bip39, kdf driver, cli;
                    vendored_math (no libm), argon2+blake2b, hkpair,
                    kch2 keychain
    vendor/petri/   verbatim engine import, provenance in VENDORED.md
    vendor/kyber/   verbatim pq-crystals kyber-768 import
    data/           bip39 english wordlist, official vector cache
    helix-gui/      egui frontend, one ffi boundary (gui_api.c)
    tests/          14 suites: kats, determinism, format, keychain
    tools/          gen_vectors.py, fuzz targets, timing harnesses
    scripts/        cross_libc.sh, cross_arch.sh, fuzz_dna.sh,
                    push_run.sh
    results/        measurement logs + release manifest
    docs/           specs, threat model, claims, limitations

## connection to petri

the engine keeps its semantics untouched: helix seeds it and reads
activations, nothing inside vendor/ is modified. the derivation is
therefore exactly as deterministic as the engine's own published
determinism gates, and any future engine determinism regression
shows up here as a kdf determinism failure.
