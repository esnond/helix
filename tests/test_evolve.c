/* evolution workflow: bootstrap, chain advance, determinism across
   keychain files, generation retrieval, r0 regression (chain gen 0
   equals the stateless derivation). */
#include <stdio.h>
#include <string.h>

#include "hex.h"
#include "kdf.h"
#include "bip39_vectors_inc.h"

#define KC1 "/tmp/helix_test_evolve1.db"
#define KC2 "/tmp/helix_test_evolve2.db"
#define SALT_A "00112233445566778899aabbccddeeff"

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

int main(void)
{
    const char* m = BIP39_CASES[0].mnemonic;
    uint8_t k0[32], k1[32], k2[32], kb[32], salt[16];
    size_t slen = 0;
    uint32_t gen = 0;
    int i;

    remove(KC1);
    remove(KC2);

    /* bootstrap: first evolve creates gens 0 and 1 */
    check(helix_evolve(m, KC1, &gen, k1, NULL) == 0 && gen == 1,
          "bootstrap to gen 1");

    /* r0 regression: chain gen 0 equals the stateless derivation */
    check(helix_derive(m, NULL, 0, NULL, k0, NULL) == 0, "r0 derive");
    check(helix_derive_generation(m, KC1, 0, kb) == 0, "fetch gen 0");
    check(memcmp(k0, kb, 32) == 0, "chain gen 0 == stateless r0");

    check(helix_evolve(m, KC1, &gen, k2, NULL) == 0 && gen == 2, "evolve gen 2");
    check(memcmp(k0, k1, 32) != 0 && memcmp(k1, k2, 32) != 0 &&
          memcmp(k0, k2, 32) != 0, "gens 0 1 2 pairwise distinct");

    for (i = 0; i < 3; i++)
        check(helix_evolve(m, KC1, &gen, kb, NULL) == 0, "evolve 3..5");
    check(gen == 5, "chain at gen 5");

    /* retrieval of every stored generation */
    for (i = 0; i <= 5; i++)
        check(helix_derive_generation(m, KC1, (uint32_t)i, kb) == 0,
              "retrieve gen");
    check(helix_derive_generation(m, KC1, 6, kb) != 0, "missing gen rejected");

    /* determinism: a fresh keychain produces the identical sequence */
    check(helix_evolve(m, KC2, &gen, kb, NULL) == 0 && gen == 1,
          "second keychain bootstrap");
    check(helix_derive_generation(m, KC2, 1, kb) == 0, "fetch kc2 gen 1");
    check(memcmp(kb, k1, 32) == 0, "same sequence bitwise");
    check(helix_derive_generation(m, KC2, 0, kb) == 0, "fetch kc2 gen 0");
    check(memcmp(kb, k0, 32) == 0, "same gen 0 bitwise");

    /* salted derive still works independently of the chain */
    hex_decode(SALT_A, salt, sizeof salt, &slen);
    check(helix_derive(m, salt, slen, NULL, kb, NULL) == 0, "salted derive");
    check(memcmp(kb, k0, 32) != 0, "salt still changes derivation");

    /* error paths */
    check(helix_evolve("not a mnemonic", KC1, &gen, kb, NULL) == -1,
          "evolve bad mnemonic");
    check(helix_derive_generation("not a mnemonic", KC1, 0, kb) == -1,
          "derive_generation bad mnemonic");
    check(helix_derive_generation(m, "/tmp/helix_no_such_dir/kch.db", 0,
                                  kb) == -3, "missing keychain rejected");

    remove(KC1);
    remove(KC2);
    printf("test_evolve: %d ok, %d failed\n", g_ok, g_fail);
    return g_fail != 0;
}
