#ifndef HELIX_ARGON2_H
#define HELIX_ARGON2_H

#include <stddef.h>
#include <stdint.h>

/* argon2id per rfc 9106, v 0x13, single threaded multi lane.
   out_len is capped at 64 (the short variable length hash); the key
   files only ever need 32 bytes. profile used by helix (rfc 9106
   section 4 "balanced"): t=3, m=64 MiB, p=4. */
#define HELIX_ARGON2_T     3u
#define HELIX_ARGON2_M_KIB 65536u
#define HELIX_ARGON2_LANES 4u

/* 0 ok; -1 bad parameters; -2 out of memory.
   full form carries optional rfc 9106 secret/associated data (NULL =
   absent, length fields still enter h0); the short form is the plain
   password hash used by helix. */
int helix_argon2id_full(const uint8_t* pwd, size_t pwd_len,
                        const uint8_t* salt, size_t salt_len,
                        const uint8_t* secret, size_t sec_len,
                        const uint8_t* ad, size_t ad_len,
                        uint32_t t, uint32_t m_kib, uint32_t lanes,
                        uint8_t* out, size_t out_len);
int helix_argon2id(const uint8_t* pwd, size_t pwd_len,
                   const uint8_t* salt, size_t salt_len,
                   uint32_t t, uint32_t m_kib, uint32_t lanes,
                   uint8_t* out, size_t out_len);

#endif
