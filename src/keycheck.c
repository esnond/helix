/* generation bound verification tag: hmac-sha512 truncated to 32.
   constant time compare because tags guard key material equality. */
#include <string.h>

#include "keycheck.h"
#include "secure_mem.h"
#include "hmac.h"

static const char DOMAIN[] = "helix-keycheck";

void keycheck_tag(const uint8_t key[32], uint32_t gen, uint8_t tag[32])
{
    hmac_sha512_ctx hc;
    uint8_t out[SHA512_DIGEST];
    uint8_t g[4];
    int i;
    g[0] = (uint8_t)(gen >> 24);
    g[1] = (uint8_t)(gen >> 16);
    g[2] = (uint8_t)(gen >> 8);
    g[3] = (uint8_t)gen;
    hmac_sha512_init(&hc, key, 32);
    hmac_sha512_update(&hc, DOMAIN, sizeof DOMAIN - 1);
    hmac_sha512_update(&hc, g, 4);
    hmac_sha512_final(&hc, out);
    for (i = 0; i < 32; i++)
        tag[i] = out[i];
    helix_memzero(&hc, sizeof hc);
    helix_memzero(out, sizeof out);
}

int keycheck_verify(const uint8_t key[32], uint32_t gen,
                    const uint8_t tag[32])
{
    uint8_t want[32];
    volatile uint8_t diff = 0;
    int i;
    keycheck_tag(key, gen, want);
    for (i = 0; i < 32; i++)
        diff |= (uint8_t)(want[i] ^ tag[i]);
    helix_memzero(want, sizeof want);
    return diff == 0;
}
