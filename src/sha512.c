/* fips 180-4 sha-512. 128 byte blocks, byte wise big endian loads, fixed
   operation order for bitwise reproducibility. */
#include <string.h>

#include "sha512.h"
#include "secure_mem.h"
#include "sha512_k.h"

static uint64_t rotr64(uint64_t x, int n)
{
    return (x >> n) | (x << (64 - n));
}

static uint64_t load64_be(const uint8_t* p)
{
    int i;
    uint64_t v = 0;
    for (i = 0; i < 8; i++)
        v = (v << 8) | (uint64_t)p[i];
    return v;
}

static void store64_be(uint8_t* p, uint64_t v)
{
    int i;
    for (i = 0; i < 8; i++)
        p[i] = (uint8_t)(v >> (56 - 8 * i));
}

static void sha512_compress(sha512_ctx* c, const uint8_t* block)
{
    uint64_t w[80];
    uint64_t a, b, cc, d, e, f, g, h;
    int i;
    for (i = 0; i < 16; i++)
        w[i] = load64_be(block + 8 * i);
    for (i = 16; i < 80; i++) {
        uint64_t s0 = rotr64(w[i - 15], 1) ^ rotr64(w[i - 15], 8) ^ (w[i - 15] >> 7);
        uint64_t s1 = rotr64(w[i - 2], 19) ^ rotr64(w[i - 2], 61) ^ (w[i - 2] >> 6);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    a = c->h[0]; b = c->h[1]; cc = c->h[2]; d = c->h[3];
    e = c->h[4]; f = c->h[5]; g = c->h[6]; h = c->h[7];
    for (i = 0; i < 80; i++) {
        uint64_t s1 = rotr64(e, 14) ^ rotr64(e, 18) ^ rotr64(e, 41);
        uint64_t ch = (e & f) ^ (~e & g);
        uint64_t s0 = rotr64(a, 28) ^ rotr64(a, 34) ^ rotr64(a, 39);
        uint64_t maj = (a & b) ^ (a & cc) ^ (b & cc);
        uint64_t t1 = h + s1 + ch + SHA512_K[i] + w[i];
        uint64_t t2 = s0 + maj;
        h = g; g = f; f = e; e = d + t1;
        d = cc; cc = b; b = a; a = t1 + t2;
    }
    c->h[0] += a; c->h[1] += b; c->h[2] += cc; c->h[3] += d;
    c->h[4] += e; c->h[5] += f; c->h[6] += g; c->h[7] += h;
}

void sha512_init(sha512_ctx* c)
{
    int i;
    for (i = 0; i < 8; i++)
        c->h[i] = SHA512_IV[i];
    c->len = 0;
    c->buf_len = 0;
}

void sha512_update(sha512_ctx* c, const void* data, size_t n)
{
    const uint8_t* p = data;
    c->len += n;
    if (c->buf_len > 0) {
        size_t take = SHA512_BLOCK - c->buf_len;
        if (take > n)
            take = n;
        memcpy(c->buf + c->buf_len, p, take);
        c->buf_len += take;
        p += take;
        n -= take;
        if (c->buf_len == SHA512_BLOCK) {
            sha512_compress(c, c->buf);
            c->buf_len = 0;
        }
    }
    while (n >= SHA512_BLOCK) {
        sha512_compress(c, p);
        p += SHA512_BLOCK;
        n -= SHA512_BLOCK;
    }
    if (n > 0) {
        memcpy(c->buf, p, n);
        c->buf_len = n;
    }
}

void sha512_final(sha512_ctx* c, uint8_t out[SHA512_DIGEST])
{
    /* 16 byte length field, high half always zero under the 2^61 bound */
    uint64_t bits = c->len * 8;
    uint64_t save = c->len;
    uint8_t tail[SHA512_BLOCK + 16];
    size_t tail_len = (c->buf_len < 112) ? SHA512_BLOCK - c->buf_len
                                         : 2 * SHA512_BLOCK - c->buf_len;
    int i;
    helix_memzero(tail, sizeof tail);
    tail[0] = 0x80;
    for (i = 0; i < 8; i++)
        tail[tail_len - 8 + i] = (uint8_t)(bits >> (56 - 8 * i));
    sha512_update(c, tail, tail_len);
    c->len = save;
    for (i = 0; i < 8; i++)
        store64_be(out + 8 * i, c->h[i]);
}

void sha512(const void* data, size_t n, uint8_t out[SHA512_DIGEST])
{
    sha512_ctx c;
    sha512_init(&c);
    sha512_update(&c, data, n);
    sha512_final(&c, out);
}
