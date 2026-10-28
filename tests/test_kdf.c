/* kdf tests: bitwise determinism (same mnemonic twice), distinctness
   (different mnemonic, different salt), cell count contract, timing.
   each derivation runs the full 600k iteration pbkdf2 plus 10000 sim
   steps, roughly three seconds. */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "hex.h"
#include "kdf.h"
#include "bip39_vectors_inc.h"

#define SALT_A "00112233445566778899aabbccddeeff"
#define SALT_B "fedcba9876543210fedcba9876543210"
/* herm_302.csv: 302 neurons plus 146 muscle/passive targets = 448
   unique nodes, all driven as cells by the engine */
#define EXPECTED_CELLS 448
/* frozen r0 derivation, vector 0 mnemonic + SALT_A; any change here is
   an r0 breaking change and must bump the format version */
#define EXPECTED_KEY "4b6d7dcd2c168ad86b7274f2bf5cb902141d9983493d6d1267bff08526a9f8b2"

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

static double now_s(void)
{
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

int main(void)
{
    uint8_t salt_a[16], salt_b[16];
    size_t slen = 0;
    uint8_t k1[HELIX_DERIVED_KEY_LEN], k2[HELIX_DERIVED_KEY_LEN];
    uint8_t k3[HELIX_DERIVED_KEY_LEN];
    helix_stats st;
    double t0, t1;

    hex_decode(SALT_A, salt_a, sizeof salt_a, &slen);
    hex_decode(SALT_B, salt_b, sizeof salt_b, &slen);

    t0 = now_s();
    check(helix_derive(BIP39_CASES[0].mnemonic, salt_a, sizeof salt_a,
                       NULL, k1, &st) == 0, "derive case 0");
    t1 = now_s();
    printf("derive: %d cells, %ld steps, %.2f s\n", st.n_cells, st.steps,
           t1 - t0);
    check(st.n_cells == EXPECTED_CELLS, "cell count contract");

    check(helix_derive(BIP39_CASES[0].mnemonic, salt_a, sizeof salt_a,
                       NULL, k2, &st) == 0, "derive case 0 replay");
    check(memcmp(k1, k2, sizeof k1) == 0, "determinism: same bits");
    check(helix_derive(BIP39_CASES[1].mnemonic, salt_a, sizeof salt_a,
                       NULL, k3, &st) == 0, "derive case 1");
    check(memcmp(k1, k3, sizeof k1) != 0, "distinct mnemonic distinct key");
    check(helix_derive(BIP39_CASES[0].mnemonic, salt_b, sizeof salt_b,
                       NULL, k3, &st) == 0, "derive salt b");
    check(memcmp(k1, k3, sizeof k1) != 0, "distinct salt distinct key");
    check(helix_derive("not a mnemonic", salt_a, sizeof salt_a,
                       NULL, k3, &st) == -1, "bad mnemonic rejected");
    check(helix_derive(BIP39_CASES[0].mnemonic, salt_a, sizeof salt_a,
                       "data/no-such-connectome.csv", k3, &st) == -2,
          "bad connectome rejected");

    {
        char got[2 * HELIX_DERIVED_KEY_LEN + 1];
        hex_encode(k1, sizeof k1, got);
        check(strcmp(got, EXPECTED_KEY) == 0, "frozen r0 derivation pinned");
        printf("key(case0,salt_a): %s\n", got);
    }

    printf("test_kdf: %d ok, %d failed\n", g_ok, g_fail);
    return g_fail != 0;
}
