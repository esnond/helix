/* hkpair + .dna v3: external pair roundtrip, two-factor property (the
   mnemonic alone fails), tamper evidence, overwrite refusal, and v2
   backward compatibility. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "dna.h"
#include "hkpair.h"

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

#define MNEMONIC "abandon abandon abandon abandon abandon abandon " \
                 "abandon abandon abandon abandon abandon about"
#define TMPDIR "/tmp/helix_hkpair_test"

static void cleanup(void)
{
    char path[256];
    static const char* names[] = { "/in", "/v2.dna", "/v2.out", "/v3.dna",
                                   "/v3.out", "/pair.hkpair", "/pair2.hkpair",
                                   "/pair.pub", "/pairk.dna", "/pairk.out",
                                   NULL };
    int i;
    for (i = 0; names[i]; i++) {
        snprintf(path, sizeof path, TMPDIR "%s", names[i]);
        remove(path);
    }
    rmdir(TMPDIR);
}

int main(void)
{
    char in[128], v2[128], v2out[128], v3[128], v3out[128];
    char pair[128], pair2[128], pub[128], pairk[128], pairkout[128];
    FILE* f;
    snprintf(in, sizeof in, TMPDIR "/in");
    snprintf(v2, sizeof v2, TMPDIR "/v2.dna");
    snprintf(v2out, sizeof v2out, TMPDIR "/v2.out");
    snprintf(v3, sizeof v3, TMPDIR "/v3.dna");
    snprintf(v3out, sizeof v3out, TMPDIR "/v3.out");
    snprintf(pair, sizeof pair, TMPDIR "/pair.hkpair");
    snprintf(pair2, sizeof pair2, TMPDIR "/pair2.hkpair");
    snprintf(pub, sizeof pub, TMPDIR "/pair.pub");
    snprintf(pairk, sizeof pairk, TMPDIR "/pairk.dna");
    snprintf(pairkout, sizeof pairkout, TMPDIR "/pairk.out");

    cleanup();
    check(mkdir(TMPDIR, 0755) == 0, "mkdir tmpdir");
    f = fopen(in, "wb");
    check(f != NULL, "open input");
    if (f) {
        fputs("v3 external pair payload", f);
        fclose(f);
    }

    /* pair creation: exists check, bad kdf, argon2id profile */
    check(hkpair_create(pair, "pair pass phrase", HKP_KDF_PBKDF2) == 0,
          "create pbkdf2 pair");
    check(hkpair_create(pair, "again", HKP_KDF_PBKDF2) == -6,
          "refuse pair overwrite");
    check(hkpair_load(pair, "wrong pass", NULL, 0, NULL, 0, NULL) == -4,
          "wrong pair passphrase rejected");
    check(hkpair_load(pair, "pair pass phrase", NULL, 0, NULL, 0, NULL) == 0,
          "correct pair passphrase accepted");
    check(hkpair_export_pk(pair, "pair pass phrase", pub) == 0,
          "export public key");

    /* v2 still works and stays a v2 file */
    check(dna_encrypt_file(in, v2, MNEMONIC, NULL, 3) == 0,
          "v2 encrypt (no pair)");
    check(dna_decrypt_file(v2, v2out, MNEMONIC, NULL) == 0,
          "v2 decrypt");
    {
        FILE* a = fopen(in, "rb");
        FILE* b = fopen(v2out, "rb");
        int ca, cb, same = 1;
        ca = fgetc(a);
        cb = fgetc(b);
        while (ca != EOF || cb != EOF) {
            if (ca != cb) {
                same = 0;
                break;
            }
            ca = fgetc(a);
            cb = fgetc(b);
        }
        fclose(a);
        fclose(b);
        check(same, "v2 roundtrip content");
    }

    /* v3: encrypt with the external pair */
    check(dna_encrypt_file_ex(in, v3, MNEMONIC, NULL, 3, pair,
                              "pair pass phrase") == 0,
          "v3 encrypt with pair");
    check(dna_decrypt_file_ex(v3, v3out, MNEMONIC, NULL, pair,
                              "pair pass phrase") == 0,
          "v3 decrypt with pair + mnemonic");
    {
        FILE* a = fopen(in, "rb");
        FILE* b = fopen(v3out, "rb");
        int ca, cb, same = 1;
        ca = fgetc(a);
        cb = fgetc(b);
        while (ca != EOF || cb != EOF) {
            if (ca != cb) {
                same = 0;
                break;
            }
            ca = fgetc(a);
            cb = fgetc(b);
        }
        fclose(a);
        fclose(b);
        check(same, "v3 roundtrip content");
    }

    /* two-factor: without the pair, or with a wrong passphrase, the
       mnemonic alone must not decrypt */
    check(dna_decrypt_file(v3, v3out, MNEMONIC, NULL) == -4,
          "v3 refuses mnemonic-only decrypt");
    check(dna_decrypt_file_ex(v3, v3out, MNEMONIC, NULL, pair,
                              "wrong") == -4,
          "v3 refuses wrong pair passphrase");

    /* tamper: any header flip is authenticated away */
    {
        FILE* g = fopen(v3, "rb");
        long sz;
        uint8_t* buf;
        int trc;
        check(g != NULL, "reopen v3 for tamper");
        fseek(g, 0, SEEK_END);
        sz = ftell(g);
        fseek(g, 0, SEEK_SET);
        buf = malloc((size_t)sz);
        check(buf && fread(buf, 1, (size_t)sz, g) == (size_t)sz,
              "read v3 file");
        fclose(g);
        buf[10] ^= 0x40;
        g = fopen(v3, "wb");
        fwrite(buf, 1, (size_t)sz, g);
        fclose(g);
        trc = dna_decrypt_file_ex(v3, v3out, MNEMONIC, NULL, pair,
                                  "pair pass phrase");
        check(trc == -4, "v3 tamper rejected");
        free(buf);
        remove(v3);
        check(dna_encrypt_file_ex(in, v3, MNEMONIC, NULL, 3, pair,
                                  "pair pass phrase") == 0,
              "recreate v3 after tamper test");
    }

    /* overwrite refusal (A8 P2): encrypt must not clobber an output */
    check(dna_encrypt_file_ex(in, v3, MNEMONIC, NULL, 3, pair,
                              "pair pass phrase") == -6,
          "encrypt refuses existing output (v3)");
    check(dna_encrypt_file(in, v3, MNEMONIC, NULL, 3) == -6,
          "encrypt refuses existing output (v2)");

    /* a misspelled word is a distinct error (-7), not -3 */
    check(dna_encrypt_file_ex(in, pairk, "abandon abandon WRONG abandon "
                              "abandon abandon abandon abandon abandon "
                              "abandon abandon about", NULL, 3, pair,
                              "pair pass phrase") == -7,
          "invalid mnemonic is -7");

    /* a fresh argon2id pair decrypts a file encrypted to it */
    check(hkpair_create(pair2, "argon pass", HKP_KDF_ARGON2ID) == 0,
          "create argon2id pair");
    check(dna_encrypt_file_ex(in, pairk, MNEMONIC, NULL, 3, pair2,
                              "argon pass") == 0,
          "encrypt to argon2id pair");
    check(dna_decrypt_file_ex(pairk, pairkout, MNEMONIC, NULL, pair2,
                              "argon pass") == 0,
          "decrypt with argon2id pair");
    check(dna_decrypt_file_ex(pairk, pairkout, MNEMONIC, NULL, pair,
                              "pair pass phrase") == -4,
          "wrong pair (right pass) rejected");

    cleanup();
    printf("hkpair/v3: %d ok, %d fail\n", g_ok, g_fail);
    return g_fail != 0;
}
