#ifndef HELIX_KDF_H
#define HELIX_KDF_H

#include <stddef.h>
#include <stdint.h>

#define HELIX_MASTER_KEY_LEN 64
#define HELIX_DERIVED_KEY_LEN 32

typedef struct {
    int n_cells;           /* connectome cells driven */
    unsigned long iters;   /* pbkdf2 iterations */
    long steps;            /* sim steps run */
} helix_stats;

/* master key = pbkdf2_hmac_sha512(entropy, salt, 600000, 64 bytes) */
void helix_master_key(const uint8_t* entropy, size_t ent_len,
                      const uint8_t* salt, size_t salt_len,
                      uint8_t mk[HELIX_MASTER_KEY_LEN]);

/* mnemonic -> 32 byte key. the petri engine is seeded from the master
   key (rng seed from bytes 0..7, per cell membrane offset from the
   remaining 56), run 10000 steps, per step all cell activations
   (0..1, quantized to one byte) stream into sha256, and the full
   master key is folded into the digest: derived =
   sha256(trajectory || master_key). folding is required because the
   trajectory is contractive under a fixed noise stream, so membrane
   offsets alone would not keep key bytes 8..63 load bearing.
   connectome NULL selects HELIX_CONNECTOME_PATH.
   returns 0 ok, -1 bad mnemonic, -2 engine init failed. */
int helix_derive(const char* mnemonic,
                 const uint8_t* salt, size_t salt_len,
                 const char* connectome,
                 uint8_t out[HELIX_DERIVED_KEY_LEN], helix_stats* stats);

#endif
