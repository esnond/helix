/* key evolution: xor recombination plus deterministic bit mutation.
   the sequence is a pure function of the master key; no entropy is
   created after the master key exists, it is distributed across
   generations (see docs/r1_security.md for the honest claim). */
#include <string.h>

#include "evolve.h"
#include "sha256.h"

static void store32_be(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

/* seed material: master_key || u32be(gen) || domain tag */
static void mut_material(const uint8_t* master_key, size_t mk_len,
                         uint32_t gen, uint8_t mat[64 + 4 + 9])
{
    memcpy(mat, master_key, mk_len);
    store32_be(mat + mk_len, gen);
    memcpy(mat + mk_len + 4, "helix-mut", 9);
}

int evolution_recombine(const uint8_t* master_key, size_t mk_len,
                        uint32_t gen,
                        const uint8_t gen_a[32], const uint8_t gen_b[32],
                        uint8_t out[32])
{
    uint8_t mat[64 + 4 + 9];
    uint8_t blk[SHA256_DIGEST];
    uint8_t seen[32];   /* bit map of chosen bit positions */
    int i, k, chosen = 0, cursor, d;
    if (mk_len > 64)
        return -1;
    mut_material(master_key, mk_len, gen, mat);
    sha256(mat, sizeof mat, blk);
    k = 32 + (blk[0] % 33);
    for (i = 0; i < 32; i++)
        out[i] = (uint8_t)(gen_a[i] ^ gen_b[i]);
    memset(seen, 0, sizeof seen);
    cursor = 1;   /* byte 0 chose k */
    /* fixed trip count, branchless body: draws after k are no-ops, so
       the output equals the earlier early exit loop bit for bit while
       time and cache access no longer depend on the secret positions */
    for (d = 0; d < 512; d++) {
        uint8_t bit, mask, is_seen, take, apply;
        if (cursor >= SHA256_DIGEST) {
            sha256(blk, SHA256_DIGEST, blk);
            cursor = 0;
        }
        bit = blk[cursor++];
        mask = (uint8_t)(0x80u >> (bit & 7));
        is_seen = (uint8_t)((seen[bit >> 3] & mask) != 0);
        take = (uint8_t)(((1u - is_seen) & (uint8_t)(chosen < k)) & 1u);
        apply = (uint8_t)(-(int8_t)take);   /* 0x00 or 0xff */
        seen[bit >> 3] |= (uint8_t)(mask & apply);
        out[bit >> 3] ^= (uint8_t)(mask & apply);
        chosen += take;
    }
    if (chosen < k)
        return -1;   /* unreachable: 512 draws suffice with margin */
    memset(mat, 0, sizeof mat);
    memset(blk, 0, sizeof blk);
    return 0;
}

int evolution_hamming(const uint8_t a[32], const uint8_t b[32])
{
    int d = 0, i, j;
    for (i = 0; i < 32; i++) {
        uint8_t x = (uint8_t)(a[i] ^ b[i]);
        for (j = 0; j < 8; j++)
            d += (x >> j) & 1;
    }
    return d;
}
