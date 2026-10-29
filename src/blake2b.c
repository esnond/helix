/* blake2b per rfc 7693, unkeyed. argon2 depends on it (rfc 9106). */
#include <string.h>

#include "blake2b.h"

static const uint64_t IV[8] = {
    0x6a09e667f3bcc908ull, 0xbb67ae8584caa73bull,
    0x3c6ef372fe94f82bull, 0xa54ff53a5f1d36f1ull,
    0x510e527fade682d1ull, 0x9b05688c2b3e6c1full,
    0x1f83d9abfb41bd6bull, 0x5be0cd19137e2179ull
};

static const uint8_t SIGMA[10][16] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
    { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
    { 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
    { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
    { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
    { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
    { 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
    { 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
    { 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
    { 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 }
};

static uint64_t load64_le(const uint8_t* p)
{
    int i;
    uint64_t v = 0;
    for (i = 7; i >= 0; i--)
        v = (v << 8) | (uint64_t)p[i];
    return v;
}

static uint64_t rotr64(uint64_t v, unsigned n)
{
    return (v >> n) | (v << (64 - n));
}

/* rfc 7693 3.1: the message words enter only the first and third
   additions (blake2 reduced the four insertions of blake to two) */
#define G(a, b, c, d, x, y)                                    \
    do {                                                       \
        (a) = (a) + (b) + m[x];                                \
        (d) = rotr64((d) ^ (a), 32);                           \
        (c) = (c) + (d);                                       \
        (b) = rotr64((b) ^ (c), 24);                           \
        (a) = (a) + (b) + m[y];                                \
        (d) = rotr64((d) ^ (a), 16);                           \
        (c) = (c) + (d);                                       \
        (b) = rotr64((b) ^ (c), 63);                           \
    } while (0)

static void compress(blake2b_ctx* c, const uint8_t* block, int last)
{
    uint64_t m[16], v[16];
    int i, r;
    for (i = 0; i < 16; i++)
        m[i] = load64_le(block + 8 * i);
    for (i = 0; i < 8; i++)
        v[i] = c->h[i];
    for (i = 0; i < 8; i++)
        v[8 + i] = IV[i];
    v[12] ^= c->t[0];
    v[13] ^= c->t[1];
    if (last)
        v[14] = ~v[14];
    for (r = 0; r < 12; r++) {
        const uint8_t* sg = SIGMA[r % 10];
        G(v[0], v[4], v[8], v[12], sg[0], sg[1]);
        G(v[1], v[5], v[9], v[13], sg[2], sg[3]);
        G(v[2], v[6], v[10], v[14], sg[4], sg[5]);
        G(v[3], v[7], v[11], v[15], sg[6], sg[7]);
        G(v[0], v[5], v[10], v[15], sg[8], sg[9]);
        G(v[1], v[6], v[11], v[12], sg[10], sg[11]);
        G(v[2], v[7], v[8], v[13], sg[12], sg[13]);
        G(v[3], v[4], v[9], v[14], sg[14], sg[15]);
    }
    for (i = 0; i < 8; i++)
        c->h[i] ^= v[i] ^ v[i + 8];
}

void blake2b_init(blake2b_ctx* c, size_t outlen)
{
    uint8_t param[BLAKE2B_BLOCK];
    int i;
    memset(c, 0, sizeof *c);
    c->outlen = outlen;
    for (i = 0; i < 8; i++)
        c->h[i] = IV[i];
    /* rfc 7693 2.3: the parameter block is XORed into the state, it
       never goes through the compression function */
    memset(param, 0, sizeof param);
    param[0] = (uint8_t)outlen;
    param[2] = 1;
    param[3] = 1;
    for (i = 0; i < 8; i++)
        c->h[i] ^= load64_le(param + 8 * i) ^ load64_le(param + 64 + 8 * i);
}

void blake2b_update(blake2b_ctx* c, const uint8_t* data, size_t n)
{
    while (n > 0) {
        size_t take;
        if (c->buflen == BLAKE2B_BLOCK) {
            c->t[0] += BLAKE2B_BLOCK;
            if (c->t[0] < BLAKE2B_BLOCK)
                c->t[1]++;
            compress(c, c->buf, 0);
            c->buflen = 0;
        }
        take = BLAKE2B_BLOCK - c->buflen;
        if (take > n)
            take = n;
        memcpy(c->buf + c->buflen, data, take);
        c->buflen += take;
        data += take;
        n -= take;
    }
}

void blake2b_final(blake2b_ctx* c, uint8_t* out, size_t outlen)
{
    uint8_t last[BLAKE2B_BLOCK];
    int i;
    c->t[0] += c->buflen;
    if (c->t[0] < c->buflen)
        c->t[1]++;
    memset(last, 0, sizeof last);
    memcpy(last, c->buf, c->buflen);
    compress(c, last, 1);
    for (i = 0; i < (int)outlen && i < (int)c->outlen; i++)
        out[i] = (uint8_t)(c->h[i / 8] >> (8 * (i % 8)));
}

void blake2b(const uint8_t* data, size_t n, uint8_t* out, size_t outlen)
{
    blake2b_ctx c;
    blake2b_init(&c, outlen);
    blake2b_update(&c, data, n);
    blake2b_final(&c, out, outlen);
}

static void store32_le(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

void blake2b_long(uint8_t* out, size_t outlen, const uint8_t* in,
                  size_t inlen)
{
    uint8_t lenb[4], v[BLAKE2B_DIGEST];
    size_t tp;
    blake2b_ctx c;
    store32_le(lenb, (uint32_t)outlen);
    if (outlen <= BLAKE2B_DIGEST) {
        blake2b_init(&c, outlen);
        blake2b_update(&c, lenb, 4);
        blake2b_update(&c, in, inlen);
        blake2b_final(&c, out, outlen);
        return;
    }
    blake2b_init(&c, BLAKE2B_DIGEST);
    blake2b_update(&c, lenb, 4);
    blake2b_update(&c, in, inlen);
    blake2b_final(&c, v, BLAKE2B_DIGEST);
    memcpy(out, v, 32);
    out += 32;
    tp = outlen - 32;
    while (tp > BLAKE2B_DIGEST) {
        blake2b(v, BLAKE2B_DIGEST, v, BLAKE2B_DIGEST);
        memcpy(out, v, 32);
        out += 32;
        tp -= 32;
    }
    blake2b_init(&c, tp);
    blake2b_update(&c, v, BLAKE2B_DIGEST);
    blake2b_final(&c, out, tp);
}
