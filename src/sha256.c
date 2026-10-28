/* fips 180-4 sha-256. byte wise big endian loads, no host endianness
   assumption, operation order fixed for bitwise reproducibility. */
#include <string.h>

#include "sha256.h"
#include "sha256_k.h"

static uint32_t rotr32(uint32_t x, int n)
{
    return (x >> n) | (x << (32 - n));
}

static uint32_t load32_be(const uint8_t* p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void store32_be(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static void sha256_compress(sha256_ctx* c, const uint8_t* block)
{
    uint32_t w[64];
    uint32_t a, b, cc, d, e, f, g, h;
    int i;
    for (i = 0; i < 16; i++)
        w[i] = load32_be(block + 4 * i);
    for (i = 16; i < 64; i++) {
        uint32_t s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    a = c->h[0]; b = c->h[1]; cc = c->h[2]; d = c->h[3];
    e = c->h[4]; f = c->h[5]; g = c->h[6]; h = c->h[7];
    for (i = 0; i < 64; i++) {
        uint32_t s1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t s0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
        uint32_t maj = (a & b) ^ (a & cc) ^ (b & cc);
        uint32_t t1 = h + s1 + ch + SHA256_K[i] + w[i];
        uint32_t t2 = s0 + maj;
        h = g; g = f; f = e; e = d + t1;
        d = cc; cc = b; b = a; a = t1 + t2;
    }
    c->h[0] += a; c->h[1] += b; c->h[2] += cc; c->h[3] += d;
    c->h[4] += e; c->h[5] += f; c->h[6] += g; c->h[7] += h;
}

void sha256_init(sha256_ctx* c)
{
    int i;
    for (i = 0; i < 8; i++)
        c->h[i] = SHA256_IV[i];
    c->len = 0;
    c->buf_len = 0;
}

void sha256_update(sha256_ctx* c, const void* data, size_t n)
{
    const uint8_t* p = data;
    c->len += n;
    if (c->buf_len > 0) {
        size_t take = 64 - c->buf_len;
        if (take > n)
            take = n;
        memcpy(c->buf + c->buf_len, p, take);
        c->buf_len += take;
        p += take;
        n -= take;
        if (c->buf_len == 64) {
            sha256_compress(c, c->buf);
            c->buf_len = 0;
        }
    }
    while (n >= 64) {
        sha256_compress(c, p);
        p += 64;
        n -= 64;
    }
    if (n > 0) {
        memcpy(c->buf, p, n);
        c->buf_len = n;
    }
}

void sha256_final(sha256_ctx* c, uint8_t out[SHA256_DIGEST])
{
    uint64_t bits = c->len * 8;   /* input bounded by 2^61 bytes */
    uint64_t save = c->len;
    uint8_t tail[72];
    size_t tail_len = (c->buf_len < 56) ? 64 - c->buf_len : 128 - c->buf_len;
    int i;
    memset(tail, 0, sizeof tail);
    tail[0] = 0x80;
    for (i = 0; i < 8; i++)
        tail[tail_len - 8 + i] = (uint8_t)(bits >> (56 - 8 * i));
    sha256_update(c, tail, tail_len);
    c->len = save;
    for (i = 0; i < 8; i++)
        store32_be(out + 4 * i, c->h[i]);
}

void sha256(const void* data, size_t n, uint8_t out[SHA256_DIGEST])
{
    sha256_ctx c;
    sha256_init(&c);
    sha256_update(&c, data, n);
    sha256_final(&c, out);
}
