/* aead construction, rfc 8439 section 2.8. poly1305 one time key from
   chacha20 block 0; ciphertext from block counter 1; mac over aad,
   ciphertext and their lengths with the standard 16 byte padding. */
#include <string.h>

#include "aead.h"

static void pad16(poly1305_ctx* c, size_t n)
{
    static const uint8_t zeros[16] = {0};
    size_t rem = n % 16;
    if (rem)
        poly1305_update(c, zeros, 16 - rem);
}

static void le64(poly1305_ctx* c, uint64_t v)
{
    uint8_t b[8];
    int i;
    for (i = 0; i < 8; i++)
        b[i] = (uint8_t)(v >> (8 * i));
    poly1305_update(c, b, 8);
}

void chacha20_poly1305_seal(uint8_t* ct, uint8_t tag[AEAD_TAG],
                            const uint8_t* pt, size_t n,
                            const uint8_t* aad, size_t aad_len,
                            const uint8_t key[CHACHA20_KEY],
                            const uint8_t nonce[CHACHA20_NONCE])
{
    uint8_t pkey[64];
    poly1305_ctx pc;
    chacha20_block(pkey, key, nonce, 0);
    poly1305_init(&pc, pkey);
    poly1305_update(&pc, aad, aad_len);
    pad16(&pc, aad_len);
    if (n > 0) {
        memcpy(ct, pt, n);
        chacha20_encrypt(ct, n, key, nonce, 1);
        poly1305_update(&pc, ct, n);
    }
    pad16(&pc, n);
    le64(&pc, (uint64_t)aad_len);
    le64(&pc, (uint64_t)n);
    poly1305_final(&pc, tag);
    memset(pkey, 0, sizeof pkey);
    memset(&pc, 0, sizeof pc);
}

int chacha20_poly1305_open(uint8_t* pt, const uint8_t* ct, size_t n,
                           const uint8_t tag[AEAD_TAG],
                           const uint8_t* aad, size_t aad_len,
                           const uint8_t key[CHACHA20_KEY],
                           const uint8_t nonce[CHACHA20_NONCE])
{
    uint8_t pkey[64], want[AEAD_TAG];
    poly1305_ctx pc;
    volatile uint8_t diff = 0;
    int i;
    chacha20_block(pkey, key, nonce, 0);
    poly1305_init(&pc, pkey);
    poly1305_update(&pc, aad, aad_len);
    pad16(&pc, aad_len);
    poly1305_update(&pc, ct, n);
    pad16(&pc, n);
    le64(&pc, (uint64_t)aad_len);
    le64(&pc, (uint64_t)n);
    poly1305_final(&pc, want);
    memset(pkey, 0, sizeof pkey);
    memset(&pc, 0, sizeof pc);
    for (i = 0; i < AEAD_TAG; i++)
        diff |= (uint8_t)(want[i] ^ tag[i]);
    memset(want, 0, sizeof want);
    if (diff)
        return -1;
    if (n > 0) {
        memcpy(pt, ct, n);
        chacha20_encrypt(pt, n, key, nonce, 1);
    }
    return 0;
}
