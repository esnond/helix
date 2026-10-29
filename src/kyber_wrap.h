#ifndef HELIX_KYBER_WRAP_H
#define HELIX_KYBER_WRAP_H

#include <stddef.h>
#include <stdint.h>

#include "params.h"

/* kyber-768 (vendored pq-crystals reference, KYBER_K=3).

   the keypair is a pure function of the master key: randombytes is a
   shake256 stream seeded from mk, so the same mnemonic always yields the
   same pk/sk. the ciphertext is stored in the .dna header (kdf_type=1)
   and decapsulation re-derives sk from the mnemonic.

   honest boundary (docs/r3_spec.md): because sk is derived from the same
   master key as the classical share, this does NOT raise the cost for an
   adversary who has the mnemonic. it is plumbing for a future format
   where the kyber keypair is external. the security claim of r3 remains
   the pbkdf2 + chain one; see docs/security_proof.md. */
int helix_kyber_keypair(const uint8_t* mk, size_t mk_len,
                        uint8_t pk[KYBER_PUBLICKEYBYTES],
                        uint8_t sk[KYBER_SECRETKEYBYTES]);
int helix_kyber_encaps(const uint8_t* mk, size_t mk_len,
                       const uint8_t pk[KYBER_PUBLICKEYBYTES],
                       uint8_t ct[KYBER_CIPHERTEXTBYTES],
                       uint8_t ss[32]);
int helix_kyber_decaps(const uint8_t* mk, size_t mk_len,
                       const uint8_t sk[KYBER_SECRETKEYBYTES],
                       const uint8_t ct[KYBER_CIPHERTEXTBYTES],
                       uint8_t ss[32]);

/* combined = sha256(bio_share || kyber_share || "helix-r3-hybrid") */
void helix_hybrid_key(const uint8_t bio[32], const uint8_t pq[32],
                      uint8_t out[32]);

#endif
