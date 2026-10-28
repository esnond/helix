#ifndef HELIX_SHA256_H
#define HELIX_SHA256_H

#include <stddef.h>
#include <stdint.h>

#define SHA256_DIGEST 32

typedef struct {
    uint32_t h[8];
    uint64_t len;      /* total bytes fed */
    uint8_t  buf[64];
    size_t   buf_len;
} sha256_ctx;

void sha256_init(sha256_ctx* c);
void sha256_update(sha256_ctx* c, const void* data, size_t n);
void sha256_final(sha256_ctx* c, uint8_t out[SHA256_DIGEST]);
void sha256(const void* data, size_t n, uint8_t out[SHA256_DIGEST]);

#endif
