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

/* mnemonic -> master key (bip39 decode + pbkdf2).
   returns 0, -1 on bad mnemonic. */
int helix_master_key_from_mnemonic(const char* mnemonic,
                                   const uint8_t* salt, size_t salt_len,
                                   uint8_t mk[HELIX_MASTER_KEY_LEN]);

/* engine phase: petri trajectory from the master key, digest folded
   with the master key (see r0 notes in the implementation). */
int helix_engine_phase(const uint8_t mk[HELIX_MASTER_KEY_LEN],
                       const char* connectome,
                       uint8_t out[HELIX_DERIVED_KEY_LEN], helix_stats* stats);

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

/* default keychain path: $HOME/.helix-kch-<8 hex of sha256(mk)>.db.
   the fingerprint keeps different seeds in different files.
   returns 0, -1 if HOME is unset. */
int helix_keychain_path(const uint8_t mk[HELIX_MASTER_KEY_LEN],
                        char* out, size_t out_size);

/* advance the evolution chain (r1). gen 0 is the stateless r0
   derivation with an empty salt; gen 1 = recombine(gen 0, gen 0);
   gen n = recombine(gen n-1, gen n-2, master key, n). each call
   appends one generation to the keychain and returns it. the first
   call bootstraps gens 0 and 1.
   keychain_path NULL selects helix_keychain_path(mk).
   returns 0, -1 bad mnemonic, -2 engine init failed, -3 keychain
   error (see keychain.h codes), -4 HOME unset. */
int helix_evolve(const char* mnemonic, const char* keychain_path,
                 uint32_t* gen_out, uint8_t key[HELIX_DERIVED_KEY_LEN],
                 helix_stats* stats);

/* fetch an existing generation from the keychain (no derivation).
   returns 0, -1 bad mnemonic, -3 keychain error, -4 HOME unset. */
int helix_derive_generation(const char* mnemonic, const char* keychain_path,
                            uint32_t gen,
                            uint8_t key[HELIX_DERIVED_KEY_LEN]);

#endif
