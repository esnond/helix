#ifndef HELIX_HMAC_H
#define HELIX_HMAC_H

#include <stddef.h>
#include <stdint.h>

#include "sha512.h"

/* hmac-sha512, rfc 2104, block 128. midstates are cached so pbkdf2 can
   reset instead of reabsorbing the key block every iteration. */
typedef struct {
    sha512_ctx inner_init;   /* inner hash after the ipad block */
    sha512_ctx outer_init;   /* outer hash after the opad block */
    sha512_ctx inner;        /* working inner hash */
} hmac_sha512_ctx;

void hmac_sha512_init(hmac_sha512_ctx* hc, const uint8_t* key, size_t key_len);
void hmac_sha512_reset(hmac_sha512_ctx* hc);
void hmac_sha512_update(hmac_sha512_ctx* hc, const void* data, size_t n);
void hmac_sha512_final(hmac_sha512_ctx* hc, uint8_t out[SHA512_DIGEST]);

#endif
