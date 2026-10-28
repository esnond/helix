#ifndef HELIX_AEAD_H
#define HELIX_AEAD_H

#include <stddef.h>
#include <stdint.h>

#include "chacha20.h"
#include "poly1305.h"

#define AEAD_TAG POLY1305_TAG

/* rfc 8439 aead_chacha20_poly1305. ct may alias pt; tag is separate.
   seal: ct[n] = pt[n] encrypted, tag over aad||ct. */
void chacha20_poly1305_seal(uint8_t* ct, uint8_t tag[AEAD_TAG],
                            const uint8_t* pt, size_t n,
                            const uint8_t* aad, size_t aad_len,
                            const uint8_t key[CHACHA20_KEY],
                            const uint8_t nonce[CHACHA20_NONCE]);

/* 0 ok, -1 on tag mismatch (pt untouched on failure) */
int chacha20_poly1305_open(uint8_t* pt, const uint8_t* ct, size_t n,
                           const uint8_t tag[AEAD_TAG],
                           const uint8_t* aad, size_t aad_len,
                           const uint8_t key[CHACHA20_KEY],
                           const uint8_t nonce[CHACHA20_NONCE]);

#endif
