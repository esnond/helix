#ifndef HELIX_HKPAIR_H
#define HELIX_HKPAIR_H

#include <stdint.h>

/* .hkpair: external kyber-768 keypair, the v3 alternative to the
   mnemonic-derived pair (kdf_type=2). file layout:
     magic "HKP1" | version | kdf | pad[2] | profile u32be | salt[32] |
     nonce[12] | aead(pk || sk) + tag
   the payload key comes from the passphrase through the selected kdf
   (pbkdf2-hmac-sha512 with the recorded iterations, or argon2id with
   the fixed balanced profile), aad is the 54 byte header. */

#define HKP_MAGIC       "HKP1"
#define HKP_VERSION     1
#define HKP_KDF_PBKDF2  1
#define HKP_KDF_ARGON2ID 2
#define HKP_SALT_LEN    32
#define HKP_NONCE_LEN   12
#define HKP_HEADER      56

/* creates a fresh pair from os entropy. profile: argon2id -> 1,
   pbkdf2 -> HELIX_PBKDF2_ITERS. returns 0; -1 io; -3 bad kdf or
   empty passphrase; -5 out of memory; -6 file exists. */
int hkpair_create(const char* path, const char* pass, int kdf);

/* loads the pair, passphrase verified by the aead tag. pk or sk may
   be NULL when not needed. returns 0; -1 io; -2 bad format; -3
   unsupported kdf; -4 wrong passphrase; -5 out of memory. */
int hkpair_load(const char* path, const char* pass, uint8_t* pk,
                size_t pk_cap, uint8_t* sk, size_t sk_cap, int* kdf_out);

/* writes the raw public key (1184 bytes) for sharing. */
int hkpair_export_pk(const char* path, const char* pass,
                     const char* out_path);

/* profile value stored in the header for a kdf */
uint32_t hkpair_profile(int kdf);

#endif
