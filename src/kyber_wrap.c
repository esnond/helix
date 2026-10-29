#include <string.h>

#include "kem.h"
#include "kyber_wrap.h"
#include "sha256.h"

void helix_kyber_seed(const uint8_t* seed, size_t seed_len,
                      const char* mode_tag);

static void seed_for(const uint8_t* mk, size_t mk_len, const char* tag)
{
    helix_kyber_seed(mk, mk_len, tag);
}

int helix_kyber_keypair(const uint8_t* mk, size_t mk_len,
                        uint8_t pk[CRYPTO_PUBLICKEYBYTES],
                        uint8_t sk[CRYPTO_SECRETKEYBYTES])
{
    seed_for(mk, mk_len, "helix-kyber-keypair");
    return crypto_kem_keypair(pk, sk);
}

int helix_kyber_encaps(const uint8_t* mk, size_t mk_len,
                       const uint8_t pk[CRYPTO_PUBLICKEYBYTES],
                       uint8_t ct[CRYPTO_CIPHERTEXTBYTES],
                       uint8_t ss[32])
{
    seed_for(mk, mk_len, "helix-kyber-encaps");
    return crypto_kem_enc(ct, ss, pk);
}

int helix_kyber_decaps(const uint8_t* mk, size_t mk_len,
                       const uint8_t sk[CRYPTO_SECRETKEYBYTES],
                       const uint8_t ct[CRYPTO_CIPHERTEXTBYTES],
                       uint8_t ss[32])
{
    /* decaps uses sk only; the stream is seeded so a future
       implementation change cannot silently depend on os entropy */
    seed_for(mk, mk_len, "helix-kyber-decaps");
    return crypto_kem_dec(ss, ct, sk);
}

void helix_hybrid_key(const uint8_t bio[32], const uint8_t pq[32],
                      uint8_t out[32])
{
    static const char tag[] = "helix-r3-hybrid";
    uint8_t buf[64 + sizeof tag - 1];
    sha256_ctx c;
    memcpy(buf, bio, 32);
    memcpy(buf + 32, pq, 32);
    memcpy(buf + 64, tag, sizeof tag - 1);
    sha256_init(&c);
    sha256_update(&c, buf, sizeof buf);
    sha256_final(&c, out);
    memset(buf, 0, sizeof buf);
}
