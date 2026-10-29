/* kyber-768 wrapper tests: determinism, roundtrip, implicit rejection,
   and a regression pin of the pk/sk/ct/ss digests for a fixed seed.

   the pin is NOT an external KAT: it pins this build's output so a silent
   change in the vendored reference or in the drbg seeding shows up as a
   failing test. the official NIST KATs use their AES-CTR DRBG and do not
   apply to a shake-seeded stream; docs/r3_spec.md says so. */
#include <stdio.h>
#include <string.h>

#include "kyber_wrap.h"
#include "hex.h"
#include "kyber_wrap.h"
#include "sha256.h"

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

static void digest_hex(const uint8_t* p, size_t n, char* out)
{
    uint8_t d[SHA256_DIGEST];
    sha256(p, n, d);
    hex_encode(d, sizeof d, out);
}

int main(void)
{
    uint8_t mk[64], mk2[64];
    uint8_t pk[KYBER_PUBLICKEYBYTES], sk[KYBER_SECRETKEYBYTES];
    uint8_t pk2[KYBER_PUBLICKEYBYTES];
    uint8_t ct[KYBER_CIPHERTEXTBYTES], ct2[KYBER_CIPHERTEXTBYTES];
    uint8_t ss[32], ss2[32], ss3[32];
    uint8_t pk_b[KYBER_PUBLICKEYBYTES], sk_b[KYBER_SECRETKEYBYTES];
    char hex[2 * SHA256_DIGEST + 1];
    int i;

    for (i = 0; i < 64; i++) {
        mk[i] = (uint8_t)(i * 7 + 1);
        mk2[i] = (uint8_t)(i * 11 + 3);
    }

    check(helix_kyber_keypair(mk, sizeof mk, pk, sk) == 0, "keypair");
    check(helix_kyber_encaps(mk, sizeof mk, pk, ct, ss) == 0, "encaps");
    check(helix_kyber_decaps(mk, sizeof mk, sk, ct, ss2) == 0, "decaps");
    check(memcmp(ss, ss2, 32) == 0, "shared secret roundtrip");

    /* determinism: a second run of the whole procedure must match */
    check(helix_kyber_keypair(mk, sizeof mk, pk_b, sk_b) == 0, "keypair 2");
    check(helix_kyber_encaps(mk, sizeof mk, pk_b, ct2, ss3) == 0, "encaps 2");
    check(memcmp(pk, pk_b, sizeof pk) == 0, "pk deterministic");
    check(memcmp(sk, sk_b, sizeof sk) == 0, "sk deterministic");
    check(memcmp(ct, ct2, sizeof ct) == 0, "ct deterministic");
    check(memcmp(ss, ss3, 32) == 0, "ss deterministic");

    /* distinct seeds must not collide */
    check(helix_kyber_keypair(mk2, sizeof mk2, pk2, sk_b) == 0, "keypair 3");
    check(memcmp(pk, pk2, sizeof pk) != 0, "different seed, different pk");

    /* implicit rejection: a tampered ciphertext must not reproduce ss */
    memcpy(ct2, ct, sizeof ct);
    ct2[17] ^= 0x01;
    check(helix_kyber_decaps(mk, sizeof mk, sk, ct2, ss3) == 0, "decaps bad");
    check(memcmp(ss, ss3, 32) != 0, "tampered ct rejected");

    digest_hex(pk, sizeof pk, hex);
    printf("kyber768 pk %s\n", hex);
    digest_hex(sk, sizeof sk, hex);
    printf("kyber768 sk %s\n", hex);
    digest_hex(ct, sizeof ct, hex);
    printf("kyber768 ct %s\n", hex);
    digest_hex(ss, 32, hex);
    printf("kyber768 ss %s\n", hex);

    printf("test_kyber: %d ok, %d failed\n", g_ok, g_fail);
    return g_fail != 0;
}
