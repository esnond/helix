# r3 spec — .dna v2, kyber-768 hybrid

Status: research prototype. The kyber implementation is the pq-crystals
reference (unaudited, unmodified except for the randombytes source), and
no third party has reviewed the integration.

## File layout (v2)

```
offset  size   field
0       4      "DNA1"
4       1      version = 2
5       32     salt (random per file)
37      4      pbkdf2 iterations, big endian
41      4      generation, big endian
45      1      kdf_type: 0 = chain key only, 1 = chain key || kyber768 ss
46      1      cipher: 0 = chacha20-poly1305
47      48     wrapped session key (32 ct + 16 tag)
95      33     reserved, zero
128     1088   kyber-768 ciphertext (kdf_type 1 only, zero otherwise)
1216    n      body: aead(file name, size, data), aad = full 1216 header
```

The body aad covers the whole header, so the kyber ciphertext is
authenticated. The session-key wrap uses aad = header[0..47], because the
wrapped key cannot cover its own ciphertext; tampering with the kyber
ciphertext still changes the derived key and the body tag fails.

## Key derivation

```
gen_key  = chain generation key from the mnemonic (src/kdf.c, docs/security_proof.md)
(pk, sk) = kyber768.keypair(seed = gen_key)      deterministic, shake256 stream
ct, ss   = kyber768.encaps(pk)                   deterministic from gen_key
kek      = sha256(gen_key || ss || "helix-r3-hybrid")
```

Decryption re-derives `sk` from the mnemonic, decapsulates `ct`, rebuilds
`kek`. Nothing but the mnemonic and the file is needed, which is HX-7.

## What this does and does not buy

**Does:** the file format and the key schedule already accept a
post-quantum share, so a future format can store a kyber public key that
is *not* derived from the mnemonic without another break. [v1.0: that
future format shipped — .dna v3 with the external .hkpair pair, see
docs/dna_format.md. the "does not" below still describes v2 exactly.] It also
exercises the hybrid combiner and pins its behaviour.

**Does not:** raise the attack cost against an adversary who has the
mnemonic, because `sk` is derived from the same `gen_key`. A quantum
adversary who can run Grover on the mnemonic entropy gets the same
advantage with or without kyber. The effective security of a 12-word
mnemonic remains at most 64-bit post-quantum / 128-bit classical
(docs/security_proof.md section 5). **No text may claim r3 is
"quantum-safe"**; it is "hybrid-ready plumbing with an honest ceiling".

## Evidence

- `tests/test_kyber.c`: determinism of pk/sk/ct/ss, roundtrip,
  implicit rejection of a tampered ciphertext, regression pin of the
  four digests. 14 checks.
- `tests/test_dna.c`: v2 roundtrip at generation 5, tamper rejection,
  oversized-generation refusal.
- `tools/fuzz_crypto.py` covers the classical primitives, not kyber.

## Known gaps

- The vendored kyber is the reference C implementation: not constant-time
  by proof, only by construction (its own README makes the same
  statement). No side-channel measurement was done on it.
- The kyber ciphertext is deterministic for a given (mnemonic,
  generation), so two files encrypted at the same generation carry the
  same `ct`. That leaks generation equality, which the header already
  reveals in clear.
- v1 files are refused (`-2`), not migrated. Migration is "rebuild at tag
  `r2-anchor` and decrypt there".
