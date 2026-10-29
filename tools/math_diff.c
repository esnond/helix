/* math_diff: measure src/vendored_math against the libm double result.
   the reference is exp/log/sin/cos/sqrt evaluated in double (libm double
   functions are sub-ulp accurate over the tested domain), rounded to
   float, and compared by ulp distance to the vendored float result.

   usage: ./tools/math_diff [samples]
   prints one line per function, exits 1 if any function exceeds its
   documented bound (1 ulp for transcendentals, 0 for sqrt). */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vmath.h"

enum { DOM_EXP, DOM_LOG, DOM_SQRT, DOM_TRIG };

static uint64_t rng_state = 0x68656c69782d7233ULL;

static uint64_t next_u64(void)
{
    rng_state ^= rng_state >> 12;
    rng_state ^= rng_state << 25;
    rng_state ^= rng_state >> 27;
    return rng_state * 2685821657736338717ULL;
}

static float rand_float(void)
{
    uint32_t bits = (uint32_t)(next_u64() >> 32);
    float f;
    memcpy(&f, &bits, sizeof f);
    return f;
}

/* ulp distance in the float bit ordering */
static long ulp_diff(float a, float b)
{
    uint32_t x, y;
    long d;
    memcpy(&x, &a, sizeof x);
    memcpy(&y, &b, sizeof y);
    d = (long)(x & 0x7fffffffu) - (long)(y & 0x7fffffffu);
    if (x & 0x80000000u)
        d = -d;
    if (y & 0x80000000u)
        d = -d;
    return d < 0 ? -d : d;
}

static int in_domain(float x, int dom)
{
    if (x != x)
        return 0;
    switch (dom) {
    case DOM_EXP: return x <= 88.0f && x >= -87.0f;
    case DOM_LOG: return x > 0.0f && x < 1e30f;
    case DOM_SQRT: return x < 1e30f && x > -1e30f;
    default: return x <= 1e4f && x >= -1e4f;
    }
}

static long worst_ulps(float (*impl)(float), double (*ref)(double), int dom,
                       long n)
{
    long i, worst = 0;
    for (i = 0; i < n; i++) {
        float x = rand_float();
        long d;
        if (!in_domain(x, dom))
            continue;
        if (dom == DOM_SQRT)
            x = (x < 0.0f) ? -x : x;
        d = ulp_diff(impl(x), (float)ref((double)x));
        if (d > worst)
            worst = d;
    }
    return worst;
}

static int report(const char* name, long worst, long bound)
{
    printf("%-6s max ulp error %ld (bound %ld) %s\n", name, worst, bound,
           worst <= bound ? "pass" : "FAIL");
    return worst <= bound;
}

int main(int argc, char** argv)
{
    long n = 1000000, ok = 1;
    if (argc > 1) {
        n = atol(argv[1]);
        if (n < 1)
            n = 1;
    }
    ok &= report("expf", worst_ulps(vm_expf, exp, DOM_EXP, n), 1);
    ok &= report("logf", worst_ulps(vm_logf, log, DOM_LOG, n), 1);
    ok &= report("sqrtf", worst_ulps(vm_sqrtf, sqrt, DOM_SQRT, n), 0);
    ok &= report("sinf", worst_ulps(vm_sinf, sin, DOM_TRIG, n), 1);
    ok &= report("cosf", worst_ulps(vm_cosf, cos, DOM_TRIG, n), 1);
    return ok ? 0 : 1;
}
