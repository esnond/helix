#ifndef HELIX_DNA_H
#define HELIX_DNA_H

#include <stdint.h>

#define DNA_MAGIC       "DNA1"
/* v3: kdf_type=2 wraps the session key under an external kyber pair
   (.hkpair file). v2 files (chain/hybrid) stay readable; v1 files are
   refused, not mis-decrypted. */
#define DNA_VERSION     3
#define DNA_VERSION_PREV 2
#define DNA_HEADER_BASE 128
/* kyber-768 ciphertext, stored after the classical header and covered by
   the body aad. v2: the keypair is re-derived from the mnemonic (HX-7).
   v3: the pair lives in a passphrase-encrypted .hkpair file and the
   mnemonic alone can no longer decrypt. */
#define DNA_KYBER_CT_LEN 1088
#define DNA_HEADER_SIZE (DNA_HEADER_BASE + DNA_KYBER_CT_LEN)
#define DNA_SALT_LEN    32
#define DNA_ESK_LEN     64   /* 48 used: chacha20poly1305(32 B key) + tag */
#define DNA_KDF_PBKDF2  0   /* r2..: chain key only */
#define DNA_KDF_HYBRID  1   /* r3: chain key || kyber768 ss */
#define DNA_KDF_EXTERNAL 2  /* v3: external .hkpair pair, mnemonic alone
                               is not sufficient */
/* v3 header iterations field records how the pair file protects its
   secret: 1 = pbkdf2, 2 = argon2id (informational, aad-bound). */
#define DNA_PAIR_PBKDF2  1
#define DNA_PAIR_ARGON2ID 2
/* upper bound on a header generation, checked before the chain is
   recomputed: an unauthenticated generation field would otherwise buy
   an attacker unbounded cpu (2^32 sha256 recombinations). */
#define DNA_GEN_MAX     4096
#define DNA_CIPHER_CHACHA 0

/* .dna file: header(128) || aead_ciphertext(meta || file data) || tag(16).
   session key wrap: aead(gen_key, nonce = salt[0..11]) stored in header.
   body: aead(session_key, nonce = sha256(salt || "helix-dna-body")[0..11]).
   aad for both: the full 128 byte header, binding generation and
   iterations against tampering.

   gen_key comes from the evolution chain, which is a pure function of
   the mnemonic: decryption recomputes it and never needs the keychain
   (keychain_path is an optional cache). kdf_type=2 replaces the hybrid
   share with an external pair: session =
   sha256(kyber_ss || chain_key || "helix-v3-external").

   returns 0 ok; -1 io; -2 bad format; -3 generation unavailable;
   -4 authentication failed; -5 out of memory; -6 output exists
   (encrypt only). */
int dna_encrypt_file(const char* in_path, const char* out_path,
                     const char* mnemonic, const char* keychain_path,
                     uint32_t generation);
int dna_encrypt_file_ex(const char* in_path, const char* out_path,
                        const char* mnemonic, const char* keychain_path,
                        uint32_t generation, const char* pair_path,
                        const char* pair_pass);
int dna_decrypt_file(const char* in_path, const char* out_path,
                     const char* mnemonic, const char* keychain_path);
int dna_decrypt_file_ex(const char* in_path, const char* out_path,
                        const char* mnemonic, const char* keychain_path,
                        const char* pair_path, const char* pair_pass);

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
