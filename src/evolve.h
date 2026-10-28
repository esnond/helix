#ifndef HELIX_EVOLUTION_H
#define HELIX_EVOLUTION_H

#include <stddef.h>
#include <stdint.h>

/* deterministic recombination: gen_c = flip_k(xor(gen_a, gen_b)) where
   k and the flipped bit positions are pure functions of the master key
   and the generation number. the mutation seed is
   sha256(master_key || u32be(gen) || "helix-mut"); wall-clock
   timestamps are deliberately excluded (determinism-gate override of
   the r1 spec's timestamp-in-seed item, see docs/r1_security.md).
   k = 32 + seed_byte0 mod 33 (32..64 flipped bits, mean 48, matching
   the r1 spec's "1-2 bits per byte"); positions are drawn from the
   seed stream with collision skipping, one byte per draw.
   returns 0, or -1 if the position stream is exhausted (defensive,
   practically unreachable). */
int evolution_recombine(const uint8_t* master_key, size_t mk_len,
                        uint32_t gen,
                        const uint8_t gen_a[32], const uint8_t gen_b[32],
                        uint8_t out[32]);

/* popcount of a xor b, 0..256 */
int evolution_hamming(const uint8_t a[32], const uint8_t b[32]);

#endif
