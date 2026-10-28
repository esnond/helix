#ifndef HELIX_CHACHA20_H
#define HELIX_CHACHA20_H

#include <stddef.h>
#include <stdint.h>

#define CHACHA20_KEY   32
#define CHACHA20_NONCE 12

/* rfc 8439. one 64 byte keystream block for debugging and keygen */
void chacha20_block(uint8_t out[64], const uint8_t key[CHACHA20_KEY],
                    const uint8_t nonce[CHACHA20_NONCE], uint32_t counter);

/* stream xor in place, block counter starts at the given value */
void chacha20_encrypt(uint8_t* data, size_t n, const uint8_t key[CHACHA20_KEY],
                      const uint8_t nonce[CHACHA20_NONCE], uint32_t counter);

#endif
