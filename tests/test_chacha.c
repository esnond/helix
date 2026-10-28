/* rfc 8439 vector tests: block function (2.3.2), cipher (2.4.2),
   poly1305 (2.5.2), poly1305 keygen (2.6.2), aead (2.8.2). vectors
   extracted verbatim from the rfc text by tools/gen_vectors.py. */
#include <stdio.h>
#include <string.h>

#include "chacha20.h"
#include "poly1305.h"
#include "aead.h"
#include "hex.h"

#include "rfc8439_vectors_inc.h"

static int g_fail;
static int g_ok;

static void check(int ok, const char* what)
{
    if (ok) {
        g_ok++;
        return;
    }
    g_fail++;
    printf("FAIL: %s\n", what);
}

static const uint8_t KEY_00_1F[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};

int main(void)
{
    /* 2.3.2 block function */
    {
        uint8_t nonce[12] = {0x00, 0x00, 0x00, 0x09, 0x00, 0x00,
                             0x00, 0x4a, 0x00, 0x00, 0x00, 0x00};
        uint8_t out[64];
        chacha20_block(out, KEY_00_1F, nonce, 1);
        check(memcmp(out, RFC_BLOCK_OUT, 64) == 0, "rfc 2.3.2 block");
    }

    /* 2.4.2 cipher */
    {
        uint8_t buf[114];
        memcpy(buf, RFC_CIPHER_PT, 114);
        chacha20_encrypt(buf, 114, KEY_00_1F, RFC_CIPHER_NONCE, 1);
        check(memcmp(buf, RFC_CIPHER_CT, 114) == 0, "rfc 2.4.2 encrypt");
        chacha20_encrypt(buf, 114, KEY_00_1F, RFC_CIPHER_NONCE, 1);
        check(memcmp(buf, RFC_CIPHER_PT, 114) == 0, "rfc 2.4.2 decrypt");
    }

    /* 2.5.2 poly1305 */
    {
        uint8_t tag[16];
        const char* msg = "Cryptographic Forum Research Group";
        poly1305(tag, (const uint8_t*)msg, strlen(msg), RFC_POLY_KEY);
        check(memcmp(tag, RFC_POLY_TAG, 16) == 0, "rfc 2.5.2 poly1305");
    }

    /* 2.6.2 poly1305 key generation */
    {
        uint8_t out[64];
        chacha20_block(out, RFC_POLYGEN_KEY, RFC_POLYGEN_NONCE, 0);
        check(memcmp(out, RFC_POLYGEN_OUT, 32) == 0, "rfc 2.6.2 keygen");
    }

    /* 2.8.2 aead */
    {
        uint8_t ct[114], tag[16], pt[114];
        chacha20_poly1305_seal(ct, tag, RFC_AEAD_PT, 114, RFC_AEAD_AAD,
                               12, RFC_AEAD_KEY, RFC_AEAD_NONCE);
        check(memcmp(ct, RFC_AEAD_CT, 114) == 0, "rfc 2.8.2 aead encrypt");
        check(memcmp(tag, RFC_AEAD_TAG, 16) == 0, "rfc 2.8.2 aead tag");
        check(chacha20_poly1305_open(pt, RFC_AEAD_CT, 114, RFC_AEAD_TAG,
                                     RFC_AEAD_AAD, 12, RFC_AEAD_KEY,
                                     RFC_AEAD_NONCE) == 0 &&
              memcmp(pt, RFC_AEAD_PT, 114) == 0, "rfc 2.8.2 aead decrypt");
        {
            uint8_t bad[16];
            memcpy(bad, RFC_AEAD_TAG, 16);
            bad[0] ^= 1;
            check(chacha20_poly1305_open(pt, RFC_AEAD_CT, 114, bad,
                                         RFC_AEAD_AAD, 12, RFC_AEAD_KEY,
                                         RFC_AEAD_NONCE) != 0,
                  "rfc 2.8.2 aead tamper rejected");
        }
    }

    /* streaming poly update path equals one shot (split the message) */
    {
        uint8_t tag1[16], tag2[16];
        const char* msg = "Cryptographic Forum Research Group";
        size_t n = strlen(msg);
        poly1305(tag1, (const uint8_t*)msg, n, RFC_POLY_KEY);
        {
            poly1305_ctx c;
            poly1305_init(&c, RFC_POLY_KEY);
            poly1305_update(&c, (const uint8_t*)msg, 7);
            poly1305_update(&c, (const uint8_t*)msg + 7, 5);
            poly1305_update(&c, (const uint8_t*)msg + 12, n - 12);
            poly1305_final(&c, tag2);
            memset(&c, 0, sizeof c);
        }
        check(memcmp(tag1, tag2, 16) == 0, "poly1305 streaming split");
    }

    printf("test_chacha: %d ok, %d failed\n", g_ok, g_fail);
    return g_fail != 0;
}
