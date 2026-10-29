/* .dna format tests: roundtrip fidelity, backward compatibility via
   chain recomputation (no keychain), corruption rejection, metadata. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dna.h"
#include "kdf.h"
#include "bip39_vectors_inc.h"

#define M BIP39_CASES[0].mnemonic

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

static void wtest(const char* path, const char* data, size_t n)
{
    FILE* f = fopen(path, "wb");
    fwrite(data, 1, n, f);
    fclose(f);
}

static size_t rtest(const char* path, uint8_t* buf, size_t max)
{
    size_t n = 0;
    FILE* f = fopen(path, "rb");
    if (!f)
        return (size_t)-1;
    n = fread(buf, 1, max, f);
    fclose(f);
    return n;
}

int main(void)
{
    const char* payload =
        "helix dna format test payload, the quick brown worm crawls "
        "over the agar plate and reverses on tap. 0123456789";
    uint8_t back[512], *corrupt;
    size_t n;

    remove("/tmp/helix_t.dna");
    remove("/tmp/helix_t.out");
    wtest("/tmp/helix_t.in", payload, strlen(payload));

    /* roundtrip: encrypt with gen 5, decrypt with a fresh process
       state and no keychain at all - the chain is recomputed */
    check(dna_encrypt_file("/tmp/helix_t.in", "/tmp/helix_t.dna", M,
                           NULL, 5) == 0, "encrypt gen 5");
    check(dna_decrypt_file("/tmp/helix_t.dna", "/tmp/helix_t.out", M,
                           NULL) == 0, "decrypt without keychain");
    n = rtest("/tmp/helix_t.out", back, sizeof back);
    check(n == strlen(payload) && memcmp(back, payload, n) == 0,
          "roundtrip bitwise");

    /* the a8 finding: encrypt must refuse to clobber an existing
       archive instead of silently overwriting it */
    check(dna_encrypt_file("/tmp/helix_t.in", "/tmp/helix_t.dna", M,
                           NULL, 0) == -6, "encrypt refuses existing output");
    remove("/tmp/helix_t.dna");
    /* backward compatibility: gen 0 file still decrypts */
    check(dna_encrypt_file("/tmp/helix_t.in", "/tmp/helix_t.dna", M,
                           NULL, 0) == 0, "encrypt gen 0");
    check(dna_decrypt_file("/tmp/helix_t.dna", "/tmp/helix_t.out", M,
                           NULL) == 0, "decrypt gen 0");
    n = rtest("/tmp/helix_t.out", back, sizeof back);
    check(n == strlen(payload) && memcmp(back, payload, n) == 0,
          "gen 0 roundtrip");

    /* wrong mnemonic: authentication failure, not garbage */
    check(dna_decrypt_file("/tmp/helix_t.dna", "/tmp/helix_t.out",
                           BIP39_CASES[1].mnemonic, NULL) == -4,
          "wrong mnemonic rejected");

    /* corrupt header magic: format error */
    {
        size_t fl;
        uint8_t* f;
        FILE* fp = fopen("/tmp/helix_t.dna", "rb");
        fseek(fp, 0, SEEK_END);
        fl = (size_t)ftell(fp);
        fseek(fp, 0, SEEK_SET);
        f = malloc(fl);
        corrupt = malloc(fl);
        if (!f || !corrupt || fread(f, 1, fl, fp) != fl) {
            printf("FAIL: cannot read test file\n");
            return 1;
        }
        fclose(fp);
        memcpy(corrupt, f, fl);
        corrupt[0] = 'X';
        wtest("/tmp/helix_t.bad", (const char*)corrupt, fl);
        check(dna_decrypt_file("/tmp/helix_t.bad", "/tmp/helix_t.out", M,
                               NULL) == -2, "bad magic rejected");
        /* corrupt one ciphertext byte: mac failure */
        memcpy(corrupt, f, fl);
        corrupt[130] ^= 0x40;   /* first ciphertext bytes */
        wtest("/tmp/helix_t.bad", (const char*)corrupt, fl);
        check(dna_decrypt_file("/tmp/helix_t.bad", "/tmp/helix_t.out", M,
                               NULL) == -4, "corrupt body rejected");
        /* corrupt one header byte inside the aad: mac failure */
        memcpy(corrupt, f, fl);
        corrupt[6] ^= 0x01;   /* salt byte, inside the aad and nonce */
        wtest("/tmp/helix_t.bad", (const char*)corrupt, fl);
        check(dna_decrypt_file("/tmp/helix_t.bad", "/tmp/helix_t.out", M,
                               NULL) == -4, "header tamper rejected");
        /* generation outside DNA_GEN_MAX must be refused before the
           chain recompute, otherwise the unauthenticated field buys an
           attacker up to 2^32 sha256 recombinations */
        memcpy(corrupt, f, fl);
        corrupt[41] ^= 0x01;   /* generation field, now > 4096 */
        wtest("/tmp/helix_t.bad", (const char*)corrupt, fl);
        check(dna_decrypt_file("/tmp/helix_t.bad", "/tmp/helix_t.out", M,
                               NULL) == -2, "oversized generation refused");
        free(f);
        free(corrupt);
        corrupt = NULL;
    }

    /* trailing junk appended: aad covers header, mac covers ct only,
       trailing bytes after the tag are ignored by design? no - the
       file length defines ct_len, appended junk shifts nothing but
       extends ct; must fail */
    {
        FILE* fp = fopen("/tmp/helix_t.dna", "ab");
        fwrite("JUNKJUNK", 1, 8, fp);
        fclose(fp);
        check(dna_decrypt_file("/tmp/helix_t.dna", "/tmp/helix_t.out", M,
                               NULL) == -4, "appended junk rejected");
    }

    remove("/tmp/helix_t.in");
    remove("/tmp/helix_t.dna");
    remove("/tmp/helix_t.out");
    remove("/tmp/helix_t.bad");
    printf("test_dna: %d ok, %d failed\n", g_ok, g_fail);
    return g_fail != 0;
}
