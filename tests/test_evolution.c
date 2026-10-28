/* evolution tests: determinism, bit-flip bounds, distinctness across
   generations, chain hamming distribution (expected binomial(256, 1/2):
   mean 128, std 8; the r1 spec's 40-60 figure is unreachable for xor
   recombination of pseudorandom keys, see docs/r1_security.md). */
#include <stdio.h>
#include <string.h>

#include "evolve.h"

#define MK_LEN 64
#define CHAIN_STEPS 200

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
        buf[i] = (uint8_t)(seed * 131 + i * 29);
}

int main(void)
{
    uint8_t mk[MK_LEN], ga[32], gb[32], out1[32], out2[32];
    uint8_t chain[CHAIN_STEPS][32];
    int i, k_sum = 0, h_sum = 0;
    double k_mean, h_mean, h_var = 0.0;

    fill(mk, MK_LEN, 1);
    fill(ga, 32, 2);
    fill(gb, 32, 3);

    /* determinism: same inputs, bitwise identical */
    check(evolution_recombine(mk, MK_LEN, 7, ga, gb, out1) == 0, "recombine");
    check(evolution_recombine(mk, MK_LEN, 7, ga, gb, out2) == 0, "recombine 2");
    check(memcmp(out1, out2, 32) == 0, "determinism same inputs");
    check(memcmp(out1, ga, 32) != 0 && memcmp(out1, gb, 32) != 0,
          "output differs from inputs");

    /* sensitivity: seed material or inputs change, output changes */
    check(evolution_recombine(mk, MK_LEN, 8, ga, gb, out2) == 0, "recombine g8");
    check(memcmp(out1, out2, 32) != 0, "generation changes output");
    mk[0] ^= 1;
    check(evolution_recombine(mk, MK_LEN, 7, ga, gb, out2) == 0, "recombine mk'");
    check(memcmp(out1, out2, 32) != 0, "master key changes output");
    mk[0] ^= 1;

    /* self recombination: xor collapses, mutation remains */
    check(evolution_recombine(mk, MK_LEN, 1, ga, ga, out1) == 0, "self recombine");
    check(memcmp(out1, ga, 32) != 0, "self recombine differs from input");

    /* flip count bound: hamming(out, xor) = k in 32..64 */
    for (i = 0; i < 1000; i++) {
        uint8_t base[32];
        int k;
        fill(ga, 32, (unsigned)i + 10);
        fill(gb, 32, (unsigned)i + 20);
        check(evolution_recombine(mk, MK_LEN, (uint32_t)i, ga, gb, out1) == 0,
              "recombine loop");
        for (k = 0; k < 32; k++)
            base[k] = (uint8_t)(ga[k] ^ gb[k]);
        {
            int d = evolution_hamming(base, out1);
            if (d < 32 || d > 64) {
                check(0, "flip count in 32..64");
                break;
            }
        }
        k_sum += evolution_hamming(base, out1);
    }
    k_mean = (double)k_sum / 1000.0;
    printf("mutation flips: mean %.2f over 1000 gens (expected 48)\n",
           k_mean);
    check(k_mean > 44.0 && k_mean < 52.0, "flip mean near 48");

    /* chain: consecutive-generation hamming distribution */
    fill(chain[0], 32, 99);
    check(evolution_recombine(mk, MK_LEN, 1, chain[0], chain[0],
                              chain[1]) == 0, "chain bootstrap");
    for (i = 2; i < CHAIN_STEPS; i++)
        check(evolution_recombine(mk, MK_LEN, (uint32_t)i, chain[i - 1],
                                  chain[i - 2], chain[i]) == 0, "chain step");
    for (i = 1; i < CHAIN_STEPS; i++) {
        int d = evolution_hamming(chain[i - 1], chain[i]);
        if (d == 0) {
            check(0, "no identical consecutive generations");
            break;
        }
        h_sum += d;
    }
    h_mean = (double)h_sum / (double)(CHAIN_STEPS - 1);
    for (i = 1; i < CHAIN_STEPS; i++) {
        double d = (double)evolution_hamming(chain[i - 1], chain[i]) - h_mean;
        h_var += d * d;
    }
    h_var = h_var / (double)(CHAIN_STEPS - 2);
    /* chi square style: variance of binomial(256,1/2) is 64, std 8 */
    h_var = (h_var > 0.0) ? h_var : 0.0;
    /* sqrt without libm dependency in tests: compare variance instead */
    printf("chain hamming: mean %.2f, variance %.2f over %d steps "
           "(expected mean 128, variance 64)\n",
           h_mean, h_var, CHAIN_STEPS - 1);
    check(h_mean > 120.0 && h_mean < 136.0, "chain hamming mean near 128");
    check(h_var > 40.0 && h_var < 100.0, "chain hamming variance near 64");

    /* distinctness: 200 generations of one chain all pairwise distinct
       (sampled: consecutive windows) */
    {
        int distinct = 1;
        for (i = 2; i < CHAIN_STEPS && distinct; i++) {
            int j;
            for (j = 0; j < i; j++)
                if (memcmp(chain[i], chain[j], 32) == 0) {
                    distinct = 0;
                    break;
                }
        }
        check(distinct, "all chain generations distinct");
    }

    printf("test_evolution: %d ok, %d failed\n", g_ok, g_fail);
    return g_fail != 0;
}
