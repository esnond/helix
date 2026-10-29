/* poly1305 one time authenticator, rfc 8439. donna style 26 bit limbs,
   64 bit intermediates, constant time carry and finalization. */
#include <string.h>

#include "poly1305.h"
#include "secure_mem.h"

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

void poly1305_init(poly1305_ctx* c, const uint8_t key[32])
{
    c->r[0] = (load32_le(key) >> 0) & 0x3ffffffu;
    c->r[1] = (load32_le(key + 3) >> 2) & 0x3ffff03u;
    c->r[2] = (load32_le(key + 6) >> 4) & 0x3ffc0ffu;
    c->r[3] = (load32_le(key + 9) >> 6) & 0x3f03fffu;
    c->r[4] = (load32_le(key + 12) >> 8) & 0x00fffffu;
    c->h[0] = c->h[1] = c->h[2] = c->h[3] = c->h[4] = 0;
    c->pad[0] = load32_le(key + 16);
    c->pad[1] = load32_le(key + 20);
    c->pad[2] = load32_le(key + 24);
    c->pad[3] = load32_le(key + 28);
    c->leftover = 0;
    c->final = 0;
}

static void poly_block(poly1305_ctx* c, const uint8_t* p, uint8_t hibit)
{
    uint64_t t0, t1, t2, t3, t4, cc;
    uint32_t r0 = c->r[0], r1 = c->r[1], r2 = c->r[2], r3 = c->r[3],
             r4 = c->r[4];
    uint32_t s1 = r1 * 5u, s2 = r2 * 5u, s3 = r3 * 5u, s4 = r4 * 5u;
    uint32_t h0 = c->h[0], h1 = c->h[1], h2 = c->h[2], h3 = c->h[3],
             h4 = c->h[4];
    h0 += (load32_le(p) >> 0) & 0x3ffffffu;
    h1 += (load32_le(p + 3) >> 2) & 0x3ffffffu;
    h2 += (load32_le(p + 6) >> 4) & 0x3ffffffu;
    h3 += (load32_le(p + 9) >> 6) & 0x3ffffffu;
    h4 += (load32_le(p + 12) >> 8) | ((uint32_t)hibit << 24);
    t0 = (uint64_t)h0 * r0 + (uint64_t)h1 * s4 + (uint64_t)h2 * s3 +
         (uint64_t)h3 * s2 + (uint64_t)h4 * s1;
    t1 = (uint64_t)h0 * r1 + (uint64_t)h1 * r0 + (uint64_t)h2 * s4 +
         (uint64_t)h3 * s3 + (uint64_t)h4 * s2;
    t2 = (uint64_t)h0 * r2 + (uint64_t)h1 * r1 + (uint64_t)h2 * r0 +
         (uint64_t)h3 * s4 + (uint64_t)h4 * s3;
    t3 = (uint64_t)h0 * r3 + (uint64_t)h1 * r2 + (uint64_t)h2 * r1 +
         (uint64_t)h3 * r0 + (uint64_t)h4 * s4;
    t4 = (uint64_t)h0 * r4 + (uint64_t)h1 * r3 + (uint64_t)h2 * r2 +
         (uint64_t)h3 * r1 + (uint64_t)h4 * r0;
    cc = t0 >> 26;
    h0 = (uint32_t)t0 & 0x3ffffffu;
    t1 += cc; cc = t1 >> 26; h1 = (uint32_t)t1 & 0x3ffffffu;
    t2 += cc; cc = t2 >> 26; h2 = (uint32_t)t2 & 0x3ffffffu;
    t3 += cc; cc = t3 >> 26; h3 = (uint32_t)t3 & 0x3ffffffu;
    t4 += cc; cc = t4 >> 26; h4 = (uint32_t)t4 & 0x3ffffffu;
    h0 += (uint32_t)cc * 5u;
    cc = h0 >> 26;
    h0 &= 0x3ffffffu;
    h1 += cc;
    c->h[0] = h0; c->h[1] = h1; c->h[2] = h2; c->h[3] = h3; c->h[4] = h4;
}

void poly1305_update(poly1305_ctx* c, const uint8_t* msg, size_t n)
{
    if (c->leftover) {
        size_t take = 16 - c->leftover;
        if (take > n)
            take = n;
        memcpy(c->buffer + c->leftover, msg, take);
        c->leftover += take;
        msg += take;
        n -= take;
        if (c->leftover == 16) {
            poly_block(c, c->buffer, 1);
            c->leftover = 0;
        }
    }
    while (n >= 16) {
        poly_block(c, msg, 1);
        msg += 16;
        n -= 16;
    }
    if (n > 0) {
        memcpy(c->buffer, msg, n);
        c->leftover = n;
    }
}

void poly1305_final(poly1305_ctx* c, uint8_t tag[POLY1305_TAG])
{
    uint32_t g0, g1, g2, g3, g4, mask, cc;
    uint64_t f;
    uint8_t hbuf[16];
    if (c->leftover) {
        size_t i = c->leftover;
        c->buffer[i++] = 1;
        for (; i < 16; i++)
            c->buffer[i] = 0;
        c->final = 1;
        poly_block(c, c->buffer, 0);
    }
    cc = c->h[1] >> 26; c->h[1] &= 0x3ffffffu;
    c->h[2] += cc; cc = c->h[2] >> 26; c->h[2] &= 0x3ffffffu;
    c->h[3] += cc; cc = c->h[3] >> 26; c->h[3] &= 0x3ffffffu;
    c->h[4] += cc; cc = c->h[4] >> 26; c->h[4] &= 0x3ffffffu;
    c->h[0] += cc * 5u; cc = c->h[0] >> 26; c->h[0] &= 0x3ffffffu;
    c->h[1] += cc;
    g0 = c->h[0] + 5u; cc = g0 >> 26; g0 &= 0x3ffffffu;
    g1 = c->h[1] + cc; cc = g1 >> 26; g1 &= 0x3ffffffu;
    g2 = c->h[2] + cc; cc = g2 >> 26; g2 &= 0x3ffffffu;
    g3 = c->h[3] + cc; cc = g3 >> 26; g3 &= 0x3ffffffu;
    g4 = c->h[4] + cc - (1u << 26);
    mask = (g4 >> 31) - 1u;   /* all ones if h < p, zero otherwise */
    g0 &= mask; g1 &= mask; g2 &= mask; g3 &= mask; g4 &= mask;
    mask = ~mask;
    c->h[0] = (c->h[0] & mask) | g0;
    c->h[1] = (c->h[1] & mask) | g1;
    c->h[2] = (c->h[2] & mask) | g2;
    c->h[3] = (c->h[3] & mask) | g3;
    c->h[4] = (c->h[4] & mask) | g4;
    /* 5 x 26 bit limbs -> 128 bit little endian -> add pad -> tag */
    {
        /* the 0xffffffff masks are load bearing: the raw limb words
           overlap and the carry chain would double count bits 64..77 */
        uint64_t hv0 = ((uint64_t)c->h[0] | ((uint64_t)c->h[1] << 26)) & 0xffffffffull;
        uint64_t hv1 = (((uint64_t)c->h[1] >> 6) | ((uint64_t)c->h[2] << 20)) & 0xffffffffull;
        uint64_t hv2 = (((uint64_t)c->h[2] >> 12) | ((uint64_t)c->h[3] << 14)) & 0xffffffffull;
        uint64_t hv3 = (((uint64_t)c->h[3] >> 18) | ((uint64_t)c->h[4] << 8)) & 0xffffffffull;
        f = hv0 + c->pad[0];
        c->h[0] = (uint32_t)f;
        f = hv1 + c->pad[1] + (f >> 32);
        c->h[1] = (uint32_t)f;
        f = hv2 + c->pad[2] + (f >> 32);
        c->h[2] = (uint32_t)f;
        f = hv3 + c->pad[3] + (f >> 32);
        c->h[3] = (uint32_t)f;
        store32_le(hbuf, c->h[0]);
        store32_le(hbuf + 4, c->h[1]);
        store32_le(hbuf + 8, c->h[2]);
        store32_le(hbuf + 12, c->h[3]);
    }
    memcpy(tag, hbuf, POLY1305_TAG);
    helix_memzero(hbuf, sizeof hbuf);
}

void poly1305(uint8_t tag[POLY1305_TAG], const uint8_t* msg, size_t n,
              const uint8_t key[32])
{
    poly1305_ctx c;
    poly1305_init(&c, key);
    poly1305_update(&c, msg, n);
    poly1305_final(&c, tag);
    memset(&c, 0, sizeof c);
}
