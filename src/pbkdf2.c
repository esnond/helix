/* pbkdf2-hmac-sha512, rfc 8018. u chains through the cached midstate
   hmac: one reset plus one 64 byte block per iteration. */
#include <string.h>

#include "pbkdf2.h"
#include "hmac.h"

static void store32_be(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static void dk_block(hmac_sha512_ctx* hc, const uint8_t* salt,
                     size_t salt_len, uint32_t index,
                     unsigned long iters, uint8_t out[SHA512_DIGEST])
{
    uint8_t u[SHA512_DIGEST];
    uint8_t idx[4];
    unsigned long i;
    int j;
    store32_be(idx, index);
    hmac_sha512_reset(hc);
    hmac_sha512_update(hc, salt, salt_len);
    hmac_sha512_update(hc, idx, 4);
    hmac_sha512_final(hc, u);
    memcpy(out, u, SHA512_DIGEST);
    for (i = 1; i < iters; i++) {
        hmac_sha512_reset(hc);
        hmac_sha512_update(hc, u, SHA512_DIGEST);
        hmac_sha512_final(hc, u);
        for (j = 0; j < SHA512_DIGEST; j++)
            out[j] ^= u[j];
    }
    memset(u, 0, sizeof u);
}

void pbkdf2_sha512(const uint8_t* pass, size_t pass_len,
                   const uint8_t* salt, size_t salt_len,
                   unsigned long iters, uint8_t* dk, size_t dk_len)
{
    hmac_sha512_ctx hc;
    uint8_t block[SHA512_DIGEST];
    uint32_t index = 1;
    size_t done = 0;
    if (iters < 1)
        iters = 1;
    hmac_sha512_init(&hc, pass, pass_len);
    while (done < dk_len) {
        size_t take = dk_len - done;
        dk_block(&hc, salt, salt_len, index, iters, block);
        if (take > SHA512_DIGEST)
            take = SHA512_DIGEST;
        memcpy(dk + done, block, take);
        memset(block, 0, sizeof block);
        done += take;
        index++;
    }
    memset(&hc, 0, sizeof hc);
}
