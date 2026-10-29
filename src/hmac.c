/* hmac-sha512, rfc 2104. */
#include <string.h>

#include "hmac.h"
#include "secure_mem.h"

static void key_block(uint8_t block[SHA512_BLOCK],
                      const uint8_t* key, size_t key_len)
{
    helix_memzero(block, SHA512_BLOCK);
    if (key_len > SHA512_BLOCK) {
        sha512(key, key_len, block);   /* 64 byte digest, rest stays zero */
        return;
    }
    memcpy(block, key, key_len);
}

void hmac_sha512_init(hmac_sha512_ctx* hc, const uint8_t* key, size_t key_len)
{
    uint8_t block[SHA512_BLOCK];
    uint8_t pad[SHA512_BLOCK];
    size_t i;
    key_block(block, key, key_len);
    for (i = 0; i < SHA512_BLOCK; i++)
        pad[i] = block[i] ^ 0x36;
    sha512_init(&hc->inner_init);
    sha512_update(&hc->inner_init, pad, SHA512_BLOCK);
    for (i = 0; i < SHA512_BLOCK; i++)
        pad[i] = block[i] ^ 0x5c;
    sha512_init(&hc->outer_init);
    sha512_update(&hc->outer_init, pad, SHA512_BLOCK);
    helix_memzero(block, sizeof block);
    helix_memzero(pad, sizeof pad);
    hmac_sha512_reset(hc);
}

void hmac_sha512_reset(hmac_sha512_ctx* hc)
{
    hc->inner = hc->inner_init;
}

void hmac_sha512_update(hmac_sha512_ctx* hc, const void* data, size_t n)
{
    sha512_update(&hc->inner, data, n);
}

void hmac_sha512_final(hmac_sha512_ctx* hc, uint8_t out[SHA512_DIGEST])
{
    uint8_t idigest[SHA512_DIGEST];
    sha512_ctx outer = hc->outer_init;
    sha512_final(&hc->inner, idigest);
    sha512_update(&outer, idigest, SHA512_DIGEST);
    sha512_final(&outer, out);
    helix_memzero(idigest, sizeof idigest);
}
