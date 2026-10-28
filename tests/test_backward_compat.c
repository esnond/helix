/* backward compatibility: every past generation stays retrievable and
   verifiable after the chain has evolved past it. encryption itself
   lands with the r2 .dna format; this pins the generation tracking
   and key binding machinery r2 will use. */
#include <stdio.h>
#include <string.h>

#include "keychain.h"
#include "keycheck.h"
#include "evolve.h"

#define KC_PATH "/tmp/helix_test_compat.db"
#define MK_LEN  64
#define GENS    10

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

static void fill(uint8_t* buf, int n, unsigned seed)
{
    int i;
    for (i = 0; i < n; i++)
        buf[i] = (uint8_t)(seed * 127 + i * 13);
}

int main(void)
{
    const uint8_t fp[KEYCHAIN_FP_LEN] = {0x0a, 1, 2, 3, 4, 5, 6, 7};
    uint8_t mk[MK_LEN];
    uint8_t chain[GENS][32];
    uint8_t tags[GENS][32];
    uint8_t key[32];
    keychain_t kc;
    int i;

    fill(mk, MK_LEN, 42);
    fill(chain[0], 32, 1);
    check(evolution_recombine(mk, MK_LEN, 1, chain[0], chain[0],
                              chain[1]) == 0, "bootstrap gen 1");
    for (i = 2; i < GENS; i++)
        check(evolution_recombine(mk, MK_LEN, (uint32_t)i, chain[i - 1],
                                  chain[i - 2], chain[i]) == 0, "chain");
    for (i = 0; i < GENS; i++)
        keycheck_tag(chain[i], (uint32_t)i, tags[i]);

    remove(KC_PATH);
    check(keychain_open(&kc, KC_PATH, fp) == 0, "keychain open");
    for (i = 0; i < GENS; i++)
        check(keychain_append(&kc, chain[i]) == 0, "append gen");
    keychain_close(&kc);

    /* reopen as if the chain evolved long ago; every past generation
       must still verify against its own tag */
    check(keychain_open(&kc, KC_PATH, fp) == 0, "reopen");
    for (i = 0; i < GENS; i++) {
        check(keychain_get(&kc, (uint32_t)i, key) == 0 &&
              memcmp(key, chain[i], 32) == 0, "old gen retrievable");
        check(keycheck_verify(key, (uint32_t)i, tags[i]),
              "old gen tag verifies");
    }
    /* wrong generation fails (tag binds key to generation) */
    check(!keycheck_verify(chain[0], 5, tags[0]), "wrong gen rejected");
    check(!keycheck_verify(chain[3], 3, tags[7]), "foreign tag rejected");
    keychain_close(&kc);

    remove(KC_PATH);
    printf("test_backward_compat: %d ok, %d failed\n", g_ok, g_fail);
    return g_fail != 0;
}
