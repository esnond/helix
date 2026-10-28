#ifndef HELIX_PBKDF2_H
#define HELIX_PBKDF2_H

#include <stddef.h>
#include <stdint.h>

/* pbkdf2-hmac-sha512, rfc 8018 section 5.2.
   iters must be >= 1, dk_len <= 64 * 2^32 - 1. */
void pbkdf2_sha512(const uint8_t* pass, size_t pass_len,
                   const uint8_t* salt, size_t salt_len,
                   unsigned long iters, uint8_t* dk, size_t dk_len);

#endif
