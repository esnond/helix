#ifndef HELIX_DNA_H
#define HELIX_DNA_H

#include <stdint.h>

#define DNA_MAGIC       "DNA1"
#define DNA_VERSION     1
#define DNA_HEADER_SIZE 128
#define DNA_SALT_LEN    32
#define DNA_ESK_LEN     64   /* 48 used: chacha20poly1305(32 B key) + tag */
#define DNA_KDF_PBKDF2  0
#define DNA_CIPHER_CHACHA 0

/* .dna file: header(128) || aead_ciphertext(meta || file data) || tag(16).
   session key wrap: aead(gen_key, nonce = salt[0..11]) stored in header.
   body: aead(session_key, nonce = sha256(salt || "helix-dna-body")[0..11]).
   aad for both: the full 128 byte header, binding generation and
   iterations against tampering.

   gen_key comes from the evolution chain, which is a pure function of
   the mnemonic: decryption recomputes it and never needs the keychain
   (keychain_path is an optional cache).

   returns 0 ok; -1 io; -2 bad format; -3 generation unavailable;
   -4 authentication failed; -5 out of memory. */
int dna_encrypt_file(const char* in_path, const char* out_path,
                     const char* mnemonic, const char* keychain_path,
                     uint32_t generation);
int dna_decrypt_file(const char* in_path, const char* out_path,
                     const char* mnemonic, const char* keychain_path);

/* parsed header for cli reporting */
typedef struct {
    uint8_t  salt[DNA_SALT_LEN];
    uint32_t iterations;
    uint32_t generation;
    uint8_t  kdf_type;
    uint8_t  cipher;
} dna_header_info;

/* 0 ok, -2 bad format */
int dna_parse_header(const uint8_t* hdr, dna_header_info* info);

#endif
