/* keychain kch2 tests: create, append, retrieve, persistence, tamper
   evidence (aead), wrong master key, flock exclusivity, kch1 migration
   and legacy read-only semantics, crash-tail repair, bad magic. */
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "keychain.h"
#include "sha256.h"

#define KC_PATH  "/tmp/helix_test_kch.db"
#define KC1_PATH "/tmp/helix_test_kch1.db"

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

static void fill(uint8_t* buf, uint32_t seed)
{
    int i;
    for (i = 0; i < 32; i++)
        buf[i] = (uint8_t)(seed * 31 + i * 7);
}

/* kch1 master fingerprint was sha256(mk)[0..8]; the same rule for kch2
   comes from keychain.c, the tests just derive one mk */
static uint8_t mk[64];
static uint8_t mk_other[64];

static void write_kch1(const char* path, uint8_t keys[][32], int n)
{
    FILE* f = fopen(path, "wb");
    uint8_t hdr[32];
    uint8_t rec[44];
    uint8_t digest[32];
    int i;
    sha256(mk, 64, digest);
    memset(hdr, 0, sizeof hdr);
    memcpy(hdr, "KCH1", 4);
    hdr[4] = 1;
    memcpy(hdr + 20, digest, 8);
    fwrite(hdr, 1, sizeof hdr, f);
    for (i = 0; i < n; i++) {
        uint64_t ts = 1700000000u + (uint32_t)i;
        rec[0] = 0; rec[1] = 0; rec[2] = 0; rec[3] = (uint8_t)i;
        memcpy(rec + 4, keys[i], 32);
        for (int k = 0; k < 8; k++)
            rec[36 + k] = (uint8_t)(ts >> (56 - 8 * k));
        fwrite(rec, 1, sizeof rec, f);
    }
    fclose(f);
}

int main(void)
{
    uint8_t keys[10][32];
    uint8_t got[32];
    keychain_t kc, kc2;
    int i, rc;

    sha256((const uint8_t*)"master A", 8, mk);
    sha256((const uint8_t*)"master B", 8, mk_other);
    remove(KC_PATH);
    remove(KC1_PATH);
    for (i = 0; i < 10; i++)
        fill(keys[i], (uint32_t)i);

    rc = keychain_open(&kc, KC_PATH, mk);
    check(rc == 0, "create keychain");
    check(kc.n_entries == 0 && !kc.legacy, "fresh keychain empty");
    for (i = 0; i < 10; i++)
        check(keychain_append(&kc, keys[i]) == 0, "append");
    check(kc.n_entries == 10 && kc.current_gen == 9, "counters after append");
    for (i = 0; i < 10; i++) {
        check(keychain_get(&kc, (uint32_t)i, got) == 0 &&
              memcmp(got, keys[i], 32) == 0, "get before close");
    }
    check(keychain_get(&kc, 10, got) != 0, "get past end rejected");
    keychain_close(&kc);

    /* persistence: reopen, everything intact */
    rc = keychain_open(&kc, KC_PATH, mk);
    check(rc == 0, "reopen");
    check(kc.n_entries == 10 && kc.current_gen == 9, "counters after reopen");
    for (i = 0; i < 10; i++) {
        check(keychain_get(&kc, (uint32_t)i, got) == 0 &&
              memcmp(got, keys[i], 32) == 0, "get after reopen");
    }
    keychain_close(&kc);

    /* flock: a second handle on the same file is refused while the
       first is open */
    rc = keychain_open(&kc, KC_PATH, mk);
    check(rc == 0, "open for flock test");
    rc = keychain_open(&kc2, KC_PATH, mk);
    check(rc == -6, "second handle refused (flock)");
    keychain_close(&kc);
    rc = keychain_open(&kc2, KC_PATH, mk);
    check(rc == 0, "open after close ok");
    keychain_close(&kc2);

    /* wrong master key: fingerprint mismatch */
    rc = keychain_open(&kc, KC_PATH, mk_other);
    check(rc == -3, "wrong master key rejected");
    keychain_close(&kc);

    /* tamper: a flipped ciphertext byte must fail authentication */
    {
        FILE* f = fopen(KC_PATH, "r+b");
        long sz;
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        fseek(f, sz - 1, SEEK_SET);
        {
            int c = fgetc(f);
            fseek(f, sz - 1, SEEK_SET);
            fputc(c ^ 0x40, f);
        }
        fclose(f);
    }
    rc = keychain_open(&kc, KC_PATH, mk);
    check(rc == -4, "tampered body rejected");
    keychain_close(&kc);

    /* rebuild a clean chain for the crash-tail test */
    remove(KC_PATH);
    rc = keychain_open(&kc, KC_PATH, mk);
    check(rc == 0, "recreate after tamper");
    for (i = 0; i < 10; i++)
        keychain_append(&kc, keys[i]);
    keychain_close(&kc);

    /* crash tail: garbage after the authenticated body is ignored and
       cleaned, the chain stays usable */
    {
        FILE* f = fopen(KC_PATH, "ab");
        fwrite("GARBAGE", 1, 7, f);
        fclose(f);
    }
    rc = keychain_open(&kc, KC_PATH, mk);
    check(rc == 0 && kc.n_entries == 10, "crash tail truncated");
    check(keychain_append(&kc, keys[9]) == 0, "append after cleanup");
    keychain_close(&kc);

    /* bad magic */
    {
        FILE* f = fopen(KC_PATH, "r+b");
        fseek(f, 0, SEEK_SET);
        fwrite("XXXX", 1, 4, f);
        fclose(f);
    }
    rc = keychain_open(&kc, KC_PATH, mk);
    check(rc == -2, "bad magic rejected");
    keychain_close(&kc);

    /* legacy kch1: opens read-only, migrates on first append, history
       and timestamps preserved */
    write_kch1(KC1_PATH, keys, 5);
    rc = keychain_open(&kc, KC1_PATH, mk);
    check(rc == 0 && kc.legacy && kc.n_entries == 5, "kch1 opens read-only");
    for (i = 0; i < 5; i++) {
        check(keychain_get(&kc, (uint32_t)i, got) == 0 &&
              memcmp(got, keys[i], 32) == 0, "kch1 entry intact");
    }
    check(keychain_append(&kc, keys[5]) == 0, "append migrates kch1");
    check(!kc.legacy && kc.n_entries == 6, "migrated in place");
    keychain_close(&kc);
    rc = keychain_open(&kc, KC1_PATH, mk);
    check(rc == 0 && !kc.legacy && kc.n_entries == 6, "migration persisted");
    for (i = 0; i < 6; i++) {
        check(keychain_get(&kc, (uint32_t)i, got) == 0 &&
              memcmp(got, keys[i], 32) == 0, "post-migration history");
    }
    keychain_close(&kc);
    {
        FILE* f = fopen(KC1_PATH, "rb");
        char magic[4];
        fread(magic, 1, 4, f);
        fclose(f);
        check(memcmp(magic, "KCH2", 4) == 0, "file is kch2 now");
    }
    /* explicit migration path: open, migrate, no append */
    remove(KC1_PATH);
    write_kch1(KC1_PATH, keys, 3);
    rc = keychain_open(&kc, KC1_PATH, mk);
    check(rc == 0 && kc.legacy, "second kch1 opens");
    rc = keychain_migrate(&kc);
    check(rc == 0 && !kc.legacy && kc.n_entries == 3, "explicit migrate");
    keychain_close(&kc);
    /* wrong-key kch1: fingerprint mismatch, not a crash */
    remove(KC1_PATH);
    write_kch1(KC1_PATH, keys, 3);
    rc = keychain_open(&kc, KC1_PATH, mk_other);
    check(rc == -3, "kch1 wrong key rejected");
    keychain_close(&kc);

    remove(KC_PATH);
    remove(KC1_PATH);
    printf("test_keychain: %d ok, %d failed\n", g_ok, g_fail);
    return g_fail != 0;
}
