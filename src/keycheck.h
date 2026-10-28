#ifndef HELIX_KEYCHECK_H
#define HELIX_KEYCHECK_H

#include <stdint.h>

/* verification tag binding a key to its generation. r1 stand in for
   the real .dna mac (r2 replaces this with poly1305 over ciphertext);
   same construction so generation tracking tests transfer. */
void keycheck_tag(const uint8_t key[32], uint32_t gen, uint8_t tag[32]);

/* constant time compare, 1 ok 0 mismatch */
int keycheck_verify(const uint8_t key[32], uint32_t gen,
                    const uint8_t tag[32]);

#endif
