#ifndef HELIX_SHA512_H
#define HELIX_SHA512_H

#include <stddef.h>
#include <stdint.h>

#define SHA512_DIGEST 64
#define SHA512_BLOCK  128

typedef struct {
    uint64_t h[8];
    uint64_t len;      /* total bytes fed, input bounded by 2^61 bytes */
    uint8_t  buf[SHA512_BLOCK];
    size_t   buf_len;
} sha512_ctx;

void sha512_init(sha512_ctx* c);
void sha512_update(sha512_ctx* c, const void* data, size_t n);
void sha512_final(sha512_ctx* c, uint8_t out[SHA512_DIGEST]);
void sha512(const void* data, size_t n, uint8_t out[SHA512_DIGEST]);

#endif
