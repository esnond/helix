# .dna format specification (frozen at r2)

version 1. any change bumps the version byte and requires a migration
note. little/big endian: all integers big endian.

## file layout

    header          128 bytes, fixed
    ciphertext      metadata block || file data, aead encrypted
    tag             16 bytes, poly1305 tag over the aead message

total = 128 + len(metadata) + len(data) + 16.

## header (128 bytes)

    offset  size  field
    0       4     magic "DNA1"
    4       1     version, 1
    5       32    salt, random per file (also the session wrap nonce,
                  bytes 0..11)
    37      4     iterations, pbkdf2 iteration count, informational
                  (the derivation uses the build constant; the field is
                  aad bound so it cannot be tampered with)
    41      4     generation, which evolution chain key wraps the
                  session key
    45      1     kdf_type, 0 = pbkdf2 hmac sha512 chain (r0/r1)
    46      1     cipher, 0 = chacha20 poly1305 (rfc 8439)
    47      64    encrypted_session_key: chacha20poly1305 of the 32
                  byte session key under the generation key, 48 bytes
                  used (32 ct + 16 tag), remaining 16 zeroed
    111     17    reserved, zeroed

## cryptography

- session wrap: aead_chacha20_poly1305(key = generation key,
  nonce = salt[0..11], aad = header bytes 0..46). the aad cannot be
  the full header because the field being encrypted lives inside it;
  the prefix pins magic, version, salt, iterations, generation and
  the algorithm bytes.
- body: aead_chacha20_poly1305(key = session key,
  nonce = sha256(salt || "helix-dna-body")[0..11],
  aad = full 128 byte header). one message covers metadata and file
  data together, so both are authenticated as one unit and the
  trailing tag covers everything.
- nonce uniqueness: the salt is 32 random bytes per file; both nonces
  are functions of the salt, so no (key, nonce) pair repeats across
  files. the session key itself is fresh per file, giving the wrap
  layer a second margin.
- generation key: pure function of the mnemonic (chain recomputation,
  see docs/r1_spec.md). the keychain is only a cache; a .dna file is
  decryptable from the mnemonic alone on any machine of the same
  build environment class.

## metadata block (plaintext before file data, encrypted with the body)

    offset  size  field
    0       2     original_filename length (u16, big endian)
    2       n     original filename, bytes only (no nul)
    2+n     8     encryption timestamp, unix seconds (u64)
    10+n    8     original file size (u64)

the decrypted size is checked against the actual ciphertext length;
mismatch is a format error.

## deviations from the r2 brief (spec bugs found before implementation)

1. the brief's header summed to 131 bytes while claiming 128; fixed
   by shrinking reserved to 17 bytes.
2. the brief allocated 64 bytes to encrypted_session_key, but
   chacha20poly1305 of a 32 byte key is exactly 48 bytes; the field
   stays 64 bytes per the frozen layout with 16 bytes zeroed.
3. the brief never defined the aead nonces; using the salt directly
   for two messages under one session key would have been nonce
   reuse. nonces are defined above.
4. the brief's decrypt workflow required the keychain; the
   implementation recomputes the generation key from the mnemonic
   (the r1 chain property) and treats the keychain as an optional
   cache. decrypt takes -m (mnemonic), which the brief omitted.

## limits

- whole file buffered in memory (r2 scope, fine to ~100 mb; streaming
  chunk mode is an r3+ option)
- the header binds generation and iterations via aad; there is no
  key rotation story for files encrypted under a generation whose
  key material is lost with the mnemonic (by design: mnemonic is the
  root of trust)
