#ifndef HELIX_POLY1305_H
#define HELIX_POLY1305_H

#include <stddef.h>
#include <stdint.h>

#define POLY1305_TAG 16

typedef struct {
    uint32_t r[5], h[5], pad[4];
    size_t   leftover;
    uint8_t  buffer[16];
    uint8_t  final;
} poly1305_ctx;

void poly1305_init(poly1305_ctx* c, const uint8_t key[32]);
void poly1305_update(poly1305_ctx* c, const uint8_t* msg, size_t n);
void poly1305_final(poly1305_ctx* c, uint8_t tag[POLY1305_TAG]);
void poly1305(uint8_t tag[POLY1305_TAG], const uint8_t* msg, size_t n,
              const uint8_t key[32]);

#endif

