#ifndef HELIX_BLAKE2B_H
#define HELIX_BLAKE2B_H

#include <stddef.h>
#include <stdint.h>

#define BLAKE2B_BLOCK  128
#define BLAKE2B_DIGEST 64

/* rfc 7693 blake2b, unkeyed, incremental. outlen 1..64. */
typedef struct {
    uint64_t h[8];
    uint64_t t[2];
    uint8_t  buf[BLAKE2B_BLOCK];
    size_t   buflen;
    size_t   outlen;
} blake2b_ctx;

void blake2b_init(blake2b_ctx* c, size_t outlen);
void blake2b_update(blake2b_ctx* c, const uint8_t* data, size_t n);
void blake2b_final(blake2b_ctx* c, uint8_t* out, size_t outlen);
void blake2b(const uint8_t* data, size_t n, uint8_t* out, size_t outlen);

/* variable length hash h' (argon2 rfc 9106 3.3.2, reference
   semantics: each 64-byte hash contributes its first 32 bytes, the
   final chunk is hashed with outlen = remaining). */
void blake2b_long(uint8_t* out, size_t outlen, const uint8_t* in,
                  size_t inlen);

#endif
