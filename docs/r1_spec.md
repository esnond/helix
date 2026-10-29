# r1 spec

**historical milestone spec** (r1, kch1). the keychain part was
superseded by kch2 in v1.0 — src/keychain.c, docs/RELEASE_v1.0.md.: key evolution

status: implemented (r1). r0 derivation unchanged and pinned
(tests/test_kdf.c, EXPECTED_KEY).

## chain definition

    gen_0 = helix_engine_phase(master_key)          # stateless r0 derive, empty salt
    gen_1 = recombine(gen_0, gen_0, master_key, 1)  # pure mutation of the mask
    gen_n = recombine(gen_{n-1}, gen_{n-2}, master_key, n), n >= 2

    recombine(a, b, mk, n) = flip_k(xor(a, b))

    mutation material = sha256(mk || u32be(n) || "helix-mut")
    k                 = 32 + (material[0] mod 33)     # 32..64, mean 48
    flipped positions = bytes of sha256(material), chained by self-hash,
                        skipping already chosen positions

every value is a pure function of the master key and the generation
number. the whole chain is bitwise reproducible from the mnemonic
alone; the keychain is a cache, not a source of truth.

## deviations from the r1 brief (gate-driven)

1. timestamp in the mutation seed. the brief specified
   sha256(master_key || generation || timestamp). wall clock in the
   derivation breaks bitwise replay: the same mnemonic evolved on two
   machines would produce different keychains, and the keychain would
   become the only source of keys. determinism-gate (priority 2)
   overrides. timestamps are stored in keychain entries as audit
   metadata only and never enter derivation.
2. bootstrap. the brief's "if n < 2: gen_n = derive_key(master_key)"
   yields gen_0 == gen_1. instead gen_1 = recombine(gen_0, gen_0):
   the xor collapses and the mask alone remains, which is distinct
   from gen_0 with probability 1 - 2^-256.
3. hamming distance target 40-60 between consecutive generations is
   mathematically unreachable for xor recombination of pseudorandom
   keys: gen_n vs gen_{n-1} is binomial(256, 1/2) - measured mean
   127.72, 95% CI [127.36, 128.08], std 8.32 over 1999 steps
   (results/r1_evolution_report.md). forcing ~48 would require
   correlated keys and would weaken backward hiding from ~2^256 mask
   security to ~2^48. the flip count itself (mutation strength) hits
   the brief's 48-bit target exactly.
4. "evolve < 100 ms" is unachievable without caching the master key on
   disk; evolve costs one pbkdf2 (about 2.8 s) plus microseconds of
   hash arithmetic. the pbkdf2 cost per evolution is a deliberate
   security property: every chain advance costs an attacker the same
   work.
5. derive keeps its stateless r0 semantics; generation fetch is
   explicit (-g N). a silent dependency on mutable home state would
   break same-flags-same-output determinism.

## storage (keychain, KCH1)

append-only file, default $HOME/.helix-kch-<8 hex of sha256(mk)>.db,
0600. header 32 bytes (magic KCH1, version, master fingerprint),
entries 44 bytes: generation u32, key 32 bytes, wall-clock timestamp
u64 (audit only). torn trailing record from a crashed append is
truncated on open; wrong magic, version or fingerprint is rejected.
~44 bytes per generation: 1000 gens = 44 KB, 10000 = 440 KB.

## workflow

    ./helix evolve -m "..."            # advance one generation
    ./helix derive -m "..."            # stateless gen 0 (r0 semantics)
    ./helix derive -m "..." -g 5       # fetch generation 5 from keychain
