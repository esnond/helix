/* ct_test: dudect-style timing test for the secret-dependent paths.
   h1.4 in docs/hypotheses.md.

   each target is timed over two classes of a secret input; the statistic
   is Welch's t on the per-class means. pass is |t| < 4.5 on every target,
   which at n traces corresponds to a minimum detectable effect
   d = t*sqrt(2/n). the harness is validated by a positive control that
   MUST fail (a deliberate branch with a 10x cost difference); if the
   positive control does not show |t| > 10 the run is void.

   scope: timing only. cache side channels are out of scope; src/evolve.c
   indexes seen[]/out[] with secret-derived bit positions and that is
   reported as a known limitation, not measured here.

   usage: ./tools/ct_test [traces]   (default 1000000) */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "evolve.h"
#include "keycheck.h"

#define MAX_TRACES 4000000

static uint64_t rng_state = 0x7431c0ffee11ULL;

static uint64_t next_u64(void)
{
    rng_state ^= rng_state >> 12;
    rng_state ^= rng_state << 25;
    rng_state ^= rng_state >> 27;
    return rng_state * 2685821657736338717ULL;
}

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

typedef struct {
    double sum[2], sumsq[2];
    long n[2];
} acc_t;

static void acc_add(acc_t* a, int cls, double v)
{
    a->sum[cls] += v;
    a->sumsq[cls] += v * v;
    a->n[cls] += 1;
}

/* welch t of the two classes; nan if either class is empty */
static double mean_of(const acc_t* a, int cls)
{
    return a->n[cls] ? a->sum[cls] / (double)a->n[cls] : 0.0;
}

static double welch_t(const acc_t* a)
{
    double m0, m1, v0, v1, se;
    if (a->n[0] < 2 || a->n[1] < 2)
        return 0.0;
    m0 = a->sum[0] / (double)a->n[0];
    m1 = a->sum[1] / (double)a->n[1];
    v0 = a->sumsq[0] / (double)a->n[0] - m0 * m0;
    v1 = a->sumsq[1] / (double)a->n[1] - m1 * m1;
    se = v0 / (double)a->n[0] + v1 / (double)a->n[1];
    if (se <= 0.0)
        return 0.0;
    return (m1 - m0) / sqrt(se);
}

/* deliberate leak: same work as a compare, but the mismatch branch pays
   a fixed extra cost, which is what a non constant time memcmp does. */
static int naive_compare(const uint8_t a[32], const uint8_t b[32], int slow)
{
    int i;
    for (i = 0; i < 32; i++) {
        if (a[i] != b[i]) {
            if (slow) {
                volatile int k = 0;
                int j;
                for (j = 0; j < 400; j++)
                    k += j;
            }
            return 0;
        }
    }
    return 1;
}

static double abs_d(double v)
{
    return v < 0.0 ? -v : v;
}

int main(int argc, char** argv)
{
    long traces = 1000000;
    acc_t key_acc, mask_acc, pos_acc, aa_acc;
    uint8_t key[32], tag[32], other[32];
    uint8_t mk[64], ga[32], gb[32], out[32];
    long i;
    int fails = 0;
    double t_key, t_mask, t_pos, t_aa;

    if (argc > 1) {
        traces = atol(argv[1]);
        if (traces < 1000)
            traces = 1000;
        if (traces > MAX_TRACES)
            traces = MAX_TRACES;
    }
    memset(&key_acc, 0, sizeof key_acc);
    memset(&mask_acc, 0, sizeof mask_acc);
    memset(&pos_acc, 0, sizeof pos_acc);
    memset(&aa_acc, 0, sizeof aa_acc);
    for (i = 0; i < 32; i++) {
        key[i] = (uint8_t)(next_u64() & 0xffu);
        tag[i] = key[i];
        other[i] = key[i];
        ga[i] = (uint8_t)(next_u64() & 0xffu);
        gb[i] = (uint8_t)(next_u64() & 0xffu);
    }
    other[17] ^= 0x01;   /* one fixed difference, position independent */

    for (i = 0; i < traces; i++) {
        /* class 0: tag matches; class 1: tag differs in one byte.
           a constant time compare must not care which */
        int cls = (int)(next_u64() & 1u);
        double t0, t1;
        t0 = now_ns();
        (void)keycheck_verify(key, 7u, cls ? other : tag);
        t1 = now_ns();
        acc_add(&key_acc, cls, t1 - t0);
    }
    t_key = welch_t(&key_acc);

    /* two prepared inputs that differ only in one secret bit; the class
       is chosen randomly, so the measurement never feeds the branch */
    memset(mk, 0x5a, sizeof mk);
    for (i = 0; i < traces / 4; i++) {
        int cls = (int)(next_u64() & 1u);
        uint8_t gx[32];
        double t0, t1;
        memcpy(gx, ga, sizeof gx);
        gx[0] = (uint8_t)((gx[0] & 0xfeu) | (unsigned)cls);
        t0 = now_ns();
        (void)evolution_recombine(mk, sizeof mk, 100u, gx, gb, out);
        t1 = now_ns();
        acc_add(&mask_acc, cls, t1 - t0);
    }
    t_mask = welch_t(&mask_acc);

    for (i = 0; i < traces / 4; i++) {
        int cls = (int)(next_u64() & 1u);
        double t0, t1;
        t0 = now_ns();
        (void)naive_compare(key, other, cls);
        t1 = now_ns();
        acc_add(&pos_acc, cls, t1 - t0);
    }
    t_pos = welch_t(&pos_acc);

    for (i = 0; i < traces / 4; i++) {
        int cls = (int)(next_u64() & 1u);
        double t0, t1;
        t0 = now_ns();
        (void)keycheck_verify(key, 7u, tag);
        t1 = now_ns();
        acc_add(&aa_acc, cls, t1 - t0);
    }
    t_aa = welch_t(&aa_acc);

    printf("traces: %ld (+%ld for mask/controls)\n", traces, traces / 4);
    printf("keycheck_verify   n=%ld/%ld mean %.0f/%.0f ns  t = %8.3f  %s\n",
           key_acc.n[0], key_acc.n[1], mean_of(&key_acc, 0),
           mean_of(&key_acc, 1), t_key, abs_d(t_key) < 4.5 ? "pass" : "FAIL");
    printf("mask apply        n=%ld/%ld mean %.0f/%.0f ns  t = %8.3f  %s\n",
           mask_acc.n[0], mask_acc.n[1], mean_of(&mask_acc, 0),
           mean_of(&mask_acc, 1), t_mask, abs_d(t_mask) < 4.5 ? "pass" : "FAIL");
    printf("a/a control       t = %8.3f  %s\n", t_aa,
           abs_d(t_aa) < 4.5 ? "pass" : "FAIL");
    printf("positive control  t = %8.3f  %s (must be > 10)\n", t_pos,
           abs_d(t_pos) > 10.0 ? "detected" : "HARNESS_INVALID");

    if (abs_d(t_key) >= 4.5)
        fails++;
    if (abs_d(t_mask) >= 4.5)
        fails++;
    if (abs_d(t_aa) >= 4.5)
        fails++;
    if (abs_d(t_pos) <= 10.0)
        fails++;
    printf("result: %s\n", fails ? "FAIL" : "pass");
    return fails ? 1 : 0;
}
