#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hkpair.h"
#include "aead.h"
#include "argon2.h"
#include "entropy.h"
#include "kyber_wrap.h"
#include "pbkdf2.h"
#include "helix_config.h"
#include "secure_mem.h"

static int read_file(const char* path, uint8_t* buf, size_t n)
{
    FILE* f = fopen(path, "rb");
    size_t got;
    int err;
    if (!f)
        return -1;
    got = fread(buf, 1, n, f);
    err = ferror(f);
    fclose(f);
    if (got != n)
        return err ? -1 : -2;
    return 0;
}

uint32_t hkpair_profile(int kdf)
{
    return (kdf == HKP_KDF_ARGON2ID) ? 1u : HELIX_PBKDF2_ITERS;
}

static int payload_key(int kdf, uint32_t profile, const char* pass,
                       const uint8_t salt[HKP_SALT_LEN], uint8_t key[32])
{
    if (pass[0] == 0)
        return -3;
    if (kdf == HKP_KDF_ARGON2ID) {
        if (profile != 1u)
            return -3;
        return helix_argon2id((const uint8_t*)pass, strlen(pass), salt,
                              HKP_SALT_LEN, HELIX_ARGON2_T,
                              HELIX_ARGON2_M_KIB, HELIX_ARGON2_LANES, key,
                              32);
    }
    if (kdf != HKP_KDF_PBKDF2 || profile != HELIX_PBKDF2_ITERS)
        return -3;
    pbkdf2_sha512((const uint8_t*)pass, strlen(pass), salt,
                       HKP_SALT_LEN, profile, key, 32);
    return 0;
}

int hkpair_create(const char* path, const char* pass, int kdf)
{
    uint8_t hdr[HKP_HEADER], key[32], seed[32];
    const size_t secret_len = KYBER_PUBLICKEYBYTES + KYBER_SECRETKEYBYTES;
    const size_t seal_len = secret_len + AEAD_TAG;
    uint8_t *payload, *seal;
    FILE* f;
    int rc;
    if (kdf != HKP_KDF_PBKDF2 && kdf != HKP_KDF_ARGON2ID)
        return -3;
    if (!pass || pass[0] == 0)
        return -3;
    f = fopen(path, "rb");
    if (f) {
        fclose(f);
        return -6;
    }
    payload = malloc(KYBER_PUBLICKEYBYTES + KYBER_SECRETKEYBYTES);
    seal = malloc(KYBER_PUBLICKEYBYTES + KYBER_SECRETKEYBYTES + AEAD_TAG);
    if (!payload || !seal) {
        free(payload);
        free(seal);
        return -5;
    }
    if (entropy_fill(hdr + 12, HKP_SALT_LEN) != 0) {
        free(payload);
        free(seal);
        return -1;
    }
    if (entropy_fill(seed, sizeof seed) != 0 ||
        entropy_fill(payload, secret_len) != 0) {
        helix_memzero(seed, sizeof seed);
        free(payload);
        free(seal);
        return -1;
    }
    if (helix_kyber_keypair(seed, sizeof seed, payload,
                            payload + KYBER_PUBLICKEYBYTES) != 0) {
        helix_memzero(seed, sizeof seed);
        free(payload);
        free(seal);
        return -1;
    }
    helix_memzero(seed, sizeof seed);
    memcpy(hdr, HKP_MAGIC, 4);
    hdr[4] = HKP_VERSION;
    hdr[5] = (uint8_t)kdf;
    hdr[6] = 0;
    hdr[7] = 0;
    {
        uint32_t prof = hkpair_profile(kdf);
        hdr[8] = (uint8_t)(prof >> 24);
        hdr[9] = (uint8_t)(prof >> 16);
        hdr[10] = (uint8_t)(prof >> 8);
        hdr[11] = (uint8_t)prof;
    }
    if (entropy_fill(hdr + HKP_HEADER - HKP_NONCE_LEN, HKP_NONCE_LEN) != 0) {
        helix_memzero(payload, secret_len);
        free(payload);
        free(seal);
        return -1;
    }
    rc = payload_key(kdf, hkpair_profile(kdf), pass, hdr + 12, key);
    if (rc != 0) {
        helix_memzero(payload, secret_len);
        free(payload);
        free(seal);
        return rc;
    }
    chacha20_poly1305_seal(seal, seal + secret_len, payload, secret_len,
                           hdr, HKP_HEADER, key,
                           hdr + HKP_HEADER - HKP_NONCE_LEN);
    helix_memzero(key, sizeof key);
    helix_memzero(payload, secret_len);
    free(payload);
    f = fopen(path, "wb");
    if (!f) {
        helix_memzero(seal, seal_len);
        free(seal);
        return -1;
    }
    rc = 0;
    if (fwrite(hdr, 1, HKP_HEADER, f) != HKP_HEADER ||
        fwrite(seal, 1, seal_len, f) != seal_len)
        rc = -1;
    fclose(f);
    helix_memzero(seal, seal_len);
    free(seal);
    return rc;
}

int hkpair_load(const char* path, const char* pass, uint8_t* pk,
                size_t pk_cap, uint8_t* sk, size_t sk_cap, int* kdf_out)
{
    uint8_t hdr[HKP_HEADER], key[32];
    size_t body = KYBER_PUBLICKEYBYTES + KYBER_SECRETKEYBYTES + AEAD_TAG;
    uint8_t* file;
    uint8_t* plain;
    int rc;
    uint32_t profile;
    if (!pass || pass[0] == 0)
        return -3;
    file = malloc(HKP_HEADER + body);
    plain = malloc(KYBER_PUBLICKEYBYTES + KYBER_SECRETKEYBYTES);
    if (!file || !plain) {
        free(file);
        free(plain);
        return -5;
    }
    if (read_file(path, file, HKP_HEADER + body) != 0) {
        free(file);
        free(plain);
        return -1;
    }
    memcpy(hdr, file, HKP_HEADER);
    if (memcmp(hdr, HKP_MAGIC, 4) != 0 || hdr[4] != HKP_VERSION ||
        hdr[6] || hdr[7]) {
        free(file);
        free(plain);
        return -2;
    }
    profile = ((uint32_t)hdr[8] << 24) | ((uint32_t)hdr[9] << 16) |
              ((uint32_t)hdr[10] << 8) | (uint32_t)hdr[11];
    rc = payload_key(hdr[5], profile, pass, hdr + 12, key);
    if (rc != 0) {
        helix_memzero(file, HKP_HEADER + body);
        free(file);
        free(plain);
        return rc;
    }
    rc = chacha20_poly1305_open(plain, file + HKP_HEADER,
                                KYBER_PUBLICKEYBYTES + KYBER_SECRETKEYBYTES,
                                file + HKP_HEADER + KYBER_PUBLICKEYBYTES +
                                    KYBER_SECRETKEYBYTES, hdr, HKP_HEADER,
                                key, hdr + HKP_HEADER - HKP_NONCE_LEN);
    helix_memzero(key, sizeof key);
    helix_memzero(file, HKP_HEADER + body);
    free(file);
    if (rc != 0) {
        helix_memzero(plain, KYBER_PUBLICKEYBYTES + KYBER_SECRETKEYBYTES);
        free(plain);
        return -4;
    }
    if (pk) {
        if (pk_cap < KYBER_PUBLICKEYBYTES) {
            helix_memzero(plain, KYBER_PUBLICKEYBYTES + KYBER_SECRETKEYBYTES);
            free(plain);
            return -2;
        }
        memcpy(pk, plain, KYBER_PUBLICKEYBYTES);
    }
    if (sk) {
        if (sk_cap < KYBER_SECRETKEYBYTES) {
            helix_memzero(plain, KYBER_PUBLICKEYBYTES + KYBER_SECRETKEYBYTES);
            free(plain);
            return -2;
        }
        memcpy(sk, plain + KYBER_PUBLICKEYBYTES, KYBER_SECRETKEYBYTES);
    }
    if (kdf_out)
        *kdf_out = hdr[5];
    helix_memzero(plain, KYBER_PUBLICKEYBYTES + KYBER_SECRETKEYBYTES);
    free(plain);
    return 0;
}

int hkpair_export_pk(const char* path, const char* pass,
                     const char* out_path)
{
    uint8_t pk[KYBER_PUBLICKEYBYTES];
    FILE* f;
    int rc = hkpair_load(path, pass, pk, sizeof pk, NULL, 0, NULL);
    if (rc != 0)
        return rc;
    f = fopen(out_path, "wb");
    if (!f)
        return -1;
    if (fwrite(pk, 1, sizeof pk, f) != sizeof pk) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}
