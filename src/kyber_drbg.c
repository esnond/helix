/* deterministic randombytes for the vendored kyber reference.
   the reference calls randombytes for the keypair seeds and for the
   encapsulation coins. a file encryptor whose guarantee is "mnemonic
   alone decrypts anywhere" cannot take those from the OS, so they come
   from a shake256 stream keyed by the master key and a mode tag.
   helix_kyber_seed() must be called before any kem operation. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fips202.h"
#include "secure_mem.h"
#include "randombytes.h"

static keccak_state g_state;
static int g_ready;

void helix_kyber_seed(const uint8_t* seed, size_t seed_len,
                      const char* mode_tag)
{
    shake256_init(&g_state);
    shake256_absorb(&g_state, seed, seed_len);
    shake256_absorb(&g_state, (const uint8_t*)mode_tag, strlen(mode_tag));
    shake256_finalize(&g_state);
    g_ready = 1;
}

void randombytes(uint8_t* out, size_t outlen)
{
    if (!g_ready) {
        /* no silent os fallback: that would make the keypair machine
           dependent and break mnemonic-only decryption */
        memset(out, 0, outlen);
        return;
    }
    shake256_squeeze(out, outlen, &g_state);
}
