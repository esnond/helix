/* timing_bench: measures keycheck_verify with a matching tag vs a
   mismatching tag, interleaved, plus a deliberately variable positive
   control (early exit memcmp) so the harness is falsifiable, plus an
   a/a calibration slot (the identical correct call in a different
   interleaving position) to separate harness position bias from real
   timing differences.
   usage: timing_bench <iterations> <out_prefix>
   writes <prefix>_correct.csv, <prefix>_wrong.csv,
          <prefix>_correct2.csv, <prefix>_control_fast.csv,
          <prefix>_control_slow.csv */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hex.h"
#include "keycheck.h"
#include "sha256.h"

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

/* positive control: compare time depends on the first differing byte */
static int naive_compare(const uint8_t a[32], const uint8_t b[32])
{
    int i;
    for (i = 0; i < 32; i++)
        if (a[i] != b[i])
            return 0;
    return 1;
}

static FILE* open_csv(const char* prefix, const char* suffix)
{
    char path[512];
    snprintf(path, sizeof path, "%s_%s.csv", prefix, suffix);
    FILE* f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "timing_bench: cannot write %s\n", path);
        exit(1);
    }
    fprintf(f, "ns\n");
    return f;
}

int main(int argc, char* argv[])
{
    uint8_t key[32], tag_ok[32], tag_bad[32], tag_far[32];
    long n, i, warm;
    FILE *f_ok, *f_bad, *f_c2, *f_cf, *f_cs;
    if (argc != 3) {
        fprintf(stderr, "usage: timing_bench <iterations> <out_prefix>\n");
        return 1;
    }
    n = strtol(argv[1], NULL, 10);
    if (n < 100 || n > 10000000) {
        fprintf(stderr, "timing_bench: iterations out of range\n");
        return 1;
    }
    sha256("helix-timing-key", 16, key);
    keycheck_tag(key, 42, tag_ok);
    keycheck_tag(key, 43, tag_bad);   /* same hmac path, compare fails */
    memset(tag_far, 0xff, 32);        /* control slow: differs late? no,
                                         differs everywhere; naive compare
                                         exits at byte 0 */
    for (warm = 0; warm < 1000; warm++) {
        keycheck_verify(key, 42, tag_ok);
        keycheck_verify(key, 42, tag_bad);
        naive_compare(tag_ok, tag_ok);
        naive_compare(tag_ok, tag_far);
    }
    f_ok = open_csv(argv[2], "correct");
    f_bad = open_csv(argv[2], "wrong");
    f_c2 = open_csv(argv[2], "correct2");
    f_cf = open_csv(argv[2], "control_fast");
    f_cs = open_csv(argv[2], "control_slow");
    for (i = 0; i < n; i++) {
        double t0, t1;
        t0 = now_ns();
        keycheck_verify(key, 42, tag_ok);
        t1 = now_ns();
        fprintf(f_ok, "%.0f\n", t1 - t0);
        t0 = now_ns();
        keycheck_verify(key, 42, tag_bad);
        t1 = now_ns();
        fprintf(f_bad, "%.0f\n", t1 - t0);
        t0 = now_ns();
        keycheck_verify(key, 42, tag_ok);
        t1 = now_ns();
        fprintf(f_c2, "%.0f\n", t1 - t0);
        t0 = now_ns();
        naive_compare(tag_ok, tag_ok);
        t1 = now_ns();
        fprintf(f_cf, "%.0f\n", t1 - t0);
        t0 = now_ns();
        naive_compare(tag_ok, tag_far);
        t1 = now_ns();
        fprintf(f_cs, "%.0f\n", t1 - t0);
    }
    fclose(f_ok);
    fclose(f_bad);
    fclose(f_c2);
    fclose(f_cf);
    fclose(f_cs);
    memset(key, 0, sizeof key);
    memset(tag_ok, 0, sizeof tag_ok);
    memset(tag_bad, 0, sizeof tag_bad);
    return 0;
}
