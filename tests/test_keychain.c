/* keychain tests: create, append, retrieve, persistence across reopen,
   fingerprint mismatch, torn tail repair, bad magic rejection. */
#include <stdio.h>
#include <string.h>

#include "keychain.h"

#define KC_PATH "/tmp/helix_test_kch.db"

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

int main(void)
{
    const uint8_t fp[KEYCHAIN_FP_LEN] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint8_t keys[10][32];
    uint8_t got[32];
    keychain_t kc;
    int i, rc;

    remove(KC_PATH);
    for (i = 0; i < 10; i++)
        fill(keys[i], (uint32_t)i);

    rc = keychain_open(&kc, KC_PATH, fp);
    check(rc == 0, "create keychain");
    check(kc.n_entries == 0, "fresh keychain empty");
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
    rc = keychain_open(&kc, KC_PATH, fp);
    check(rc == 0, "reopen");
    check(kc.n_entries == 10 && kc.current_gen == 9, "counters after reopen");
    for (i = 0; i < 10; i++) {
        check(keychain_get(&kc, (uint32_t)i, got) == 0 &&
              memcmp(got, keys[i], 32) == 0, "get after reopen");
    }
    keychain_close(&kc);

    /* wrong fingerprint must refuse */
    rc = keychain_open(&kc, KC_PATH, fp);
    check(rc == 0, "reopen for fp test");
    keychain_close(&kc);
    {
        const uint8_t other[KEYCHAIN_FP_LEN] = {9, 9, 9, 9, 9, 9, 9, 9};
        rc = keychain_open(&kc, KC_PATH, other);
        check(rc == -3, "fingerprint mismatch rejected");
    }

    /* corruption: torn trailing record is repaired on open */
    {
        FILE* f = fopen(KC_PATH, "r+b");
        fseek(f, 0, SEEK_END);
        fwrite("GARBAGE", 1, 7, f);
        fclose(f);
    }
    rc = keychain_open(&kc, KC_PATH, fp);
    check(rc == 0 && kc.n_entries == 10, "torn tail truncated");
    keychain_close(&kc);

    /* bad magic */
    {
        FILE* f = fopen(KC_PATH, "r+b");
        fseek(f, 0, SEEK_SET);
        fwrite("XXXX", 1, 4, f);
        fclose(f);
    }
    rc = keychain_open(&kc, KC_PATH, fp);
    check(rc == -2, "bad magic rejected");

    remove(KC_PATH);
    printf("test_keychain: %d ok, %d failed\n", g_ok, g_fail);
    return g_fail != 0;
}
