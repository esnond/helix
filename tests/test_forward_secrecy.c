/* forward secrecy property tests. what is mechanically true:
   1. gen_n+1 is not the bare xor of its parents (mask is load bearing)
   2. an attacker holding the whole keychain can recover past masks
      algebraically (gen_n ^ gen_n-1 ^ gen_n-2) but cannot compute the
      next mask without the master key - wrong master material yields
      uncorrelated candidates
   3. wrong master key produces hamming-unrelated next generations
   what is NOT true is documented in docs/r1_security.md: the keychain
   stores every past key in plaintext, so "forward secrecy" holds only
   for future generations and only while the master key stays secret. */
#include <stdio.h>
#include <string.h>

#include "evolve.h"
#include "sha256.h"

#define MK_LEN 64
#define GENS   50

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
        buf[i] = (uint8_t)(seed * 101 + i * 17);
}

int main(void)
{
    uint8_t mk[MK_LEN], wrong_mk[MK_LEN];
    uint8_t chain[GENS][32];
    uint8_t bare[32];
    int i, j, ok_all = 1, h_ok = 1;
    long h_sum = 0;

    fill(mk, MK_LEN, 7);
    wrong_mk[0] = 0;
    for (i = 1; i < MK_LEN; i++)
        wrong_mk[i] = mk[i];   /* differs in first byte only */

    fill(chain[0], 32, 1);
    check(evolution_recombine(mk, MK_LEN, 1, chain[0], chain[0],
                              chain[1]) == 0, "bootstrap");
    for (i = 2; i < GENS; i++)
        check(evolution_recombine(mk, MK_LEN, (uint32_t)i, chain[i - 1],
                                  chain[i - 2], chain[i]) == 0, "chain");

    /* property 1: gen_n != bare xor of parents, mask always applied */
    for (i = 2; i < GENS; i++) {
        for (j = 0; j < 32; j++)
            bare[j] = (uint8_t)(chain[i - 1][j] ^ chain[i - 2][j]);
        if (memcmp(bare, chain[i], 32) == 0)
            ok_all = 0;
    }
    check(ok_all, "mask always changes the bare xor");

    /* property 2: dictionary of candidate master keys never predicts
       the next generation even with the full keychain known */
    {
        uint8_t dict[64][MK_LEN];
        uint8_t truth[32], cand[32], mk_sha[SHA256_DIGEST];
        int dict_n = 0, predicted = 0;
        fill(dict[dict_n], MK_LEN, 0);            /* all zero */
        dict_n++;
        memset(dict[dict_n], 0xff, MK_LEN);       /* all ff */
        dict_n++;
        fill(dict[dict_n], MK_LEN, 8);            /* mk-like, not the mk */
        dict_n++;
        memset(dict[dict_n], 0, MK_LEN);
        sha256(mk, MK_LEN, mk_sha);
        memcpy(dict[dict_n], mk_sha, SHA256_DIGEST);   /* hash of mk */
        dict_n++;
        memcpy(dict[dict_n], wrong_mk, MK_LEN);   /* one byte off */
        dict_n++;
        for (i = 0; i < GENS; i++) {              /* chain values as mk */
            memset(dict[dict_n], 0, MK_LEN);
            memcpy(dict[dict_n], chain[i], 32);
            dict_n++;
        }
        check(evolution_recombine(mk, MK_LEN, (uint32_t)GENS,
                                  chain[GENS - 1], chain[GENS - 2],
                                  truth) == 0, "true next gen");
        for (i = 0; i < dict_n; i++) {
            check(evolution_recombine(dict[i], MK_LEN, (uint32_t)GENS,
                                      chain[GENS - 1], chain[GENS - 2],
                                      cand) == 0, "dict candidate");
            if (memcmp(cand, truth, 32) == 0)
                predicted = 1;
        }
        check(!predicted, "dictionary never predicts next generation");
    }

    /* property 3: wrong master key (one byte off) yields an unrelated
       next generation, mean hamming ~ 128 */
    {
        uint8_t truth[32], guess[32];
        check(evolution_recombine(mk, MK_LEN, (uint32_t)GENS,
                                  chain[GENS - 1], chain[GENS - 2],
                                  truth) == 0, "truth next");
        check(evolution_recombine(wrong_mk, MK_LEN, (uint32_t)GENS,
                                  chain[GENS - 1], chain[GENS - 2],
                                  guess) == 0, "guess next");
        {
            int d = evolution_hamming(truth, guess);
            /* both outputs flip the same base with independent masks,
               so the distance is mask_t ^ mask_g: binomial with
               p = 2q(1-q), q = 48/256, mean ~78, std ~7.6 */
            check(d > 55 && d < 100, "wrong master uncorrelated output");
        }
    }

    /* property 4: flip bound holds along the chain */
    for (i = 2; i < GENS; i++) {
        for (j = 0; j < 32; j++)
            bare[j] = (uint8_t)(chain[i - 1][j] ^ chain[i - 2][j]);
        {
            int d = evolution_hamming(bare, chain[i]);
            if (d < 32 || d > 64)
                h_ok = 0;
            h_sum += d;
        }
    }
    check(h_ok, "flip count in 32..64 along chain");
    printf("flips along chain: mean %.2f over %d gens\n",
           (double)h_sum / (double)(GENS - 2), GENS - 2);

    printf("test_forward_secrecy: %d ok, %d failed\n", g_ok, g_fail);
    return g_fail != 0;
}
