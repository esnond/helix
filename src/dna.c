/* .dna encrypted archive format, spec frozen in docs/dna_format.md.
   two aead layers: the per file session key is wrapped under the
   generation key, file data under the session key; both carry the
   full header as aad so no header field can be flipped undetected. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dna.h"
#include "aead.h"
#include "sha256.h"
#include "entropy.h"
#include "kdf.h"
#include "helix_config.h"

static const char BODY_NONCE_TAG[] = "helix-dna-body";

static void store32_be(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static uint32_t load32_be(const uint8_t* p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void store64_be(uint8_t* p, uint64_t v)
{
    int i;
    for (i = 0; i < 8; i++)
        p[i] = (uint8_t)(v >> (56 - 8 * i));
}

static uint64_t load64_be(const uint8_t* p)
{
    int i;
    uint64_t v = 0;
    for (i = 0; i < 8; i++)
        v = (v << 8) | (uint64_t)p[i];
    return v;
}

static void body_nonce(const uint8_t salt[DNA_SALT_LEN], uint8_t nonce[12])
{
    uint8_t digest[SHA256_DIGEST];
    sha256_ctx c;
    sha256_init(&c);
    sha256_update(&c, salt, DNA_SALT_LEN);
    sha256_update(&c, BODY_NONCE_TAG, sizeof BODY_NONCE_TAG - 1);
    sha256_final(&c, digest);
    memcpy(nonce, digest, 12);
    memset(digest, 0, sizeof digest);
}

int dna_parse_header(const uint8_t* hdr, dna_header_info* info)
{
    if (memcmp(hdr, DNA_MAGIC, 4) != 0 || hdr[4] != DNA_VERSION)
        return -2;
    memcpy(info->salt, hdr + 5, DNA_SALT_LEN);
    info->iterations = load32_be(hdr + 37);
    info->generation = load32_be(hdr + 41);
    info->kdf_type = hdr[45];
    info->cipher = hdr[46];
    return 0;
}

static uint8_t* read_file(const char* path, size_t* out_len)
{
    FILE* f = fopen(path, "rb");
    uint8_t* buf;
    long size;
    if (!f)
        return NULL;
    if (fseek(f, 0, SEEK_END) != 0 || (size = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    buf = malloc((size_t)size > 0 ? (size_t)size : 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    if (fread(buf, 1, (size_t)size, f) != (size_t)size) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *out_len = (size_t)size;
    return buf;
}

static int write_file(const char* path, const uint8_t* buf, size_t n)
{
    FILE* f = fopen(path, "wb");
    if (!f)
        return -1;
    if (fwrite(buf, 1, n, f) != n) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

int dna_encrypt_file(const char* in_path, const char* out_path,
                     const char* mnemonic, const char* keychain_path,
                     uint32_t generation)
{
    uint8_t header[DNA_HEADER_SIZE];
    uint8_t gen_key[32], session[32], nonce[12];
    uint8_t *data = NULL, *out = NULL;
    size_t data_len = 0, meta_len, name_len, total;
    const char* base;
    int rc;
    if (helix_generation_key(mnemonic, keychain_path, generation,
                             gen_key) != 0)
        return -3;
    if (entropy_fill(header + 5, DNA_SALT_LEN) != 0) {
        memset(gen_key, 0, sizeof gen_key);
        return -1;
    }
    if (entropy_fill(session, 32) != 0) {
        memset(gen_key, 0, sizeof gen_key);
        return -1;
    }
    memset(header, 0, sizeof header);
    memcpy(header, DNA_MAGIC, 4);
    header[4] = DNA_VERSION;
    store32_be(header + 37, HELIX_PBKDF2_ITERS);
    store32_be(header + 41, generation);
    header[45] = DNA_KDF_PBKDF2;
    header[46] = DNA_CIPHER_CHACHA;
    /* wrap the session key, 48 bytes used of the 64 byte field.
       aad is the header prefix up to the esk field: the full header
       cannot cover its own encrypted contents */
    chacha20_poly1305_seal(header + 47, header + 47 + 32, session, 32,
                           header, 47, gen_key, header + 5);
    memset(gen_key, 0, sizeof gen_key);
    data = read_file(in_path, &data_len);
    if (!data) {
        memset(session, 0, sizeof session);
        return -1;
    }
    base = strrchr(in_path, '/');
    base = base ? base + 1 : in_path;
    name_len = strlen(base);
    if (name_len > 255) {
        free(data);
        memset(session, 0, sizeof session);
        return -1;
    }
    meta_len = 2 + name_len + 16;
    total = DNA_HEADER_SIZE + meta_len + data_len + AEAD_TAG;
    out = malloc(total);
    if (!out) {
        free(data);
        memset(session, 0, sizeof session);
        return -5;
    }
    memcpy(out, header, DNA_HEADER_SIZE);
    out[DNA_HEADER_SIZE] = (uint8_t)(name_len >> 8);
    out[DNA_HEADER_SIZE + 1] = (uint8_t)name_len;
    memcpy(out + DNA_HEADER_SIZE + 2, base, name_len);
    store64_be(out + DNA_HEADER_SIZE + 2 + name_len, (uint64_t)time(NULL));
    store64_be(out + DNA_HEADER_SIZE + 2 + name_len + 8,
               (uint64_t)data_len);
    memcpy(out + DNA_HEADER_SIZE + meta_len, data, data_len);
    memset(data, 0, data_len);
    free(data);
    body_nonce(header + 5, nonce);
    /* body plaintext sits in place behind the header; the aad is the
       finished header including the wrapped session key */
    chacha20_poly1305_seal(out + DNA_HEADER_SIZE,
                           out + DNA_HEADER_SIZE + meta_len + data_len,
                           out + DNA_HEADER_SIZE, meta_len + data_len,
                           out, DNA_HEADER_SIZE, session, nonce);
    rc = write_file(out_path, out, total);
    memset(out, 0, total);
    free(out);
    memset(session, 0, sizeof session);
    return rc;
}

int dna_decrypt_file(const char* in_path, const char* out_path,
                     const char* mnemonic, const char* keychain_path)
{
    uint8_t header[DNA_HEADER_SIZE], gen_key[32], session[32], nonce[12];
    uint8_t *file = NULL, *plain = NULL;
    dna_header_info info;
    size_t file_len, ct_len, name_len, data_len, meta_len;
    uint64_t stored_size;
    int rc = 0;
    file = read_file(in_path, &file_len);
    if (!file || file_len < DNA_HEADER_SIZE + AEAD_TAG + 3) {
        free(file);
        return file ? -2 : -1;
    }
    memcpy(header, file, DNA_HEADER_SIZE);
    if (dna_parse_header(header, &info) != 0) {
        free(file);
        return -2;
    }
    if (info.kdf_type != DNA_KDF_PBKDF2 || info.cipher != DNA_CIPHER_CHACHA) {
        free(file);
        return -2;
    }
    if (helix_generation_key(mnemonic, keychain_path, info.generation,
                             gen_key) != 0) {
        free(file);
        return -3;
    }
    if (chacha20_poly1305_open(session, header + 47, 32, header + 47 + 32,
                               header, 47, gen_key, header + 5) != 0) {
        memset(gen_key, 0, sizeof gen_key);
        free(file);
        return -4;
    }
    memset(gen_key, 0, sizeof gen_key);
    ct_len = file_len - DNA_HEADER_SIZE - AEAD_TAG;
    plain = malloc(ct_len > 0 ? ct_len : 1);
    if (!plain) {
        memset(session, 0, sizeof session);
        free(file);
        return -5;
    }
    body_nonce(header + 5, nonce);
    if (chacha20_poly1305_open(plain, file + DNA_HEADER_SIZE, ct_len,
                               file + DNA_HEADER_SIZE + ct_len, header,
                               DNA_HEADER_SIZE, session, nonce) != 0) {
        memset(session, 0, sizeof session);
        memset(plain, 0, ct_len);
        free(plain);
        free(file);
        return -4;
    }
    memset(session, 0, sizeof session);
    name_len = ((size_t)plain[0] << 8) | plain[1];
    if (2 + name_len + 16 > ct_len) {
        rc = -2;
        goto out;
    }
    stored_size = load64_be(plain + 2 + name_len + 8);
    meta_len = 2 + name_len + 16;
    data_len = ct_len - meta_len;
    if (stored_size != (uint64_t)data_len) {
        rc = -2;
        goto out;
    }
    if (write_file(out_path, plain + meta_len, data_len) != 0)
        rc = -1;
out:
    memset(plain, 0, ct_len);
    free(plain);
    free(file);
    return rc;
}
