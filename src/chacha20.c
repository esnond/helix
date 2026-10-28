/* chacha20 stream cipher, rfc 8439. little endian state words built
   byte wise, fixed operation order. */
#include <string.h>

#include "chacha20.h"

static uint32_t load32_le(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void store32_le(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

#define ROTL32(v, n) (((v) << (n)) | ((v) >> (32 - (n))))

#define QR(a, b, c, d)              \
    (a) += (b); (d) ^= (a); (d) = ROTL32((d), 16); \
    (c) += (d); (b) ^= (c); (b) = ROTL32((b), 12); \
    (a) += (b); (d) ^= (a); (d) = ROTL32((d), 8);  \
    (c) += (d); (b) ^= (c); (b) = ROTL32((b), 7);

void chacha20_block(uint8_t out[64], const uint8_t key[CHACHA20_KEY],
                    const uint8_t nonce[CHACHA20_NONCE], uint32_t counter)
{
    uint32_t s[16], w[16];
    int i;
    s[0] = 0x61707865u;
    s[1] = 0x3320646eu;
    s[2] = 0x79622d32u;
    s[3] = 0x6b206574u;
    for (i = 0; i < 8; i++)
        s[4 + i] = load32_le(key + 4 * i);
    s[12] = counter;
    s[13] = load32_le(nonce);
    s[14] = load32_le(nonce + 4);
    s[15] = load32_le(nonce + 8);
    for (i = 0; i < 16; i++)
        w[i] = s[i];
    for (i = 0; i < 10; i++) {
        QR(s[0], s[4], s[8], s[12])
        QR(s[1], s[5], s[9], s[13])
        QR(s[2], s[6], s[10], s[14])
        QR(s[3], s[7], s[11], s[15])
        QR(s[0], s[5], s[10], s[15])
        QR(s[1], s[6], s[11], s[12])
        QR(s[2], s[7], s[8], s[13])
        QR(s[3], s[4], s[9], s[14])
    }
    for (i = 0; i < 16; i++)
        store32_le(out + 4 * i, w[i] + s[i]);
}

void chacha20_encrypt(uint8_t* data, size_t n, const uint8_t key[CHACHA20_KEY],
                      const uint8_t nonce[CHACHA20_NONCE], uint32_t counter)
{
    uint8_t ks[64];
    size_t i = 0;
    while (i < n) {
        size_t take = n - i;
        int j;
        chacha20_block(ks, key, nonce, counter++);
        if (take > 64)
            take = 64;
        for (j = 0; j < (int)take; j++)
            data[i + (size_t)j] ^= ks[j];
        i += take;
    }
    memset(ks, 0, sizeof ks);
}
