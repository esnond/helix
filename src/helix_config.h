#ifndef HELIX_CONFIG_H
#define HELIX_CONFIG_H

/* all helix constants in one place. */

/* master key kdf, rfc 8018 pbkdf2-hmac-sha512 over the bip39 entropy */
#define HELIX_PBKDF2_ITERS     600000ul

/* connectome dynamics: 10000 steps at the engine dt of 1 ms */
#define HELIX_KDF_STEPS        10000
#define HELIX_INIT_SPREAD_MV   2.0f    /* max initial membrane offset, mv */

/* derivation connectome, 302 neuron hermaphrodite reconstruction */
#define HELIX_CONNECTOME_PATH  "vendor/petri/data/herm_302.csv"

#endif
