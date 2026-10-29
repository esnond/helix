/* deterministic scalar math, see vmath.h for the contract. */
#include <string.h>

#include "vmath.h"

#define SIGN32 0x80000000u
#define EXPO32 0x7f800000u
#define MANT32 0x007fffffu

static double d_from_bits(unsigned long long b)
{
    double d;
    memcpy(&d, &b, sizeof d);
    return d;
}

static unsigned long long bits_from_d(double d)
{
    unsigned long long b;
    memcpy(&b, &d, sizeof b);
    return b;
}

static float f_from_bits(unsigned int b)
{
    float f;
    memcpy(&f, &b, sizeof f);
    return f;
}

static float nan32(void)
{
    return f_from_bits(0x7fc00000u);
}

static float inf32(void)
{
    return f_from_bits(EXPO32);
}

float vm_fabsf(float x)
{
    unsigned int u;
    memcpy(&u, &x, sizeof u);
    u &= ~SIGN32;
    return f_from_bits(u);
}

double vm_fabs(double x)
{
    return d_from_bits(bits_from_d(x) & 0x7fffffffffffffffULL);
}

/* newton on top of the exponent-halving guess. the division makes each
   step quadratic; five steps leave the double result at rounding
   distance, so the float cast is correctly rounded. */
static double dsqrt(double d)
{
    double r;
    unsigned long long b;
    if (!(d > 0.0))
        return (d == 0.0) ? d : d_from_bits(0x7ff8000000000000ULL);
    b = bits_from_d(d);
    r = d_from_bits((b >> 1) + 0x1ff8000000000000ULL);
    r = 0.5 * (r + d / r);
    r = 0.5 * (r + d / r);
    r = 0.5 * (r + d / r);
    r = 0.5 * (r + d / r);
    r = 0.5 * (r + d / r);
    return r;
}

float vm_sqrtf(float x)
{
    unsigned int u;
    memcpy(&u, &x, sizeof u);
    if ((u & EXPO32) == EXPO32)
        return x + x;
    if (u & SIGN32)
        return (x == 0.0f) ? x : nan32();
    return (float)dsqrt((double)x);
}

double vm_hypot(double x, double y)
{
    double a = vm_fabs(x), b = vm_fabs(y), t;
    if (a < b) {
        t = a;
        a = b;
        b = t;
    }
    if (a == 0.0)
        return 0.0;
    b /= a;
    return a * dsqrt(1.0 + b * b);
}

float vm_hypotf(float x, float y)
{
    return (float)vm_hypot((double)x, (double)y);
}

#define LN2_HI 6.93147180369123816490e-01
#define LN2_LO 1.90821492927058770002e-10
#define INV_LN2 1.44269504088896338700e+00
#define SQRT2 1.41421356237309504880e+00

float vm_expf(float x)
{
    double d, r, p;
    long k;
    if (x > 88.73f)
        return inf32();
    if (x < -745.0f)
        return 0.0f;
    d = (double)x;
    k = (long)(d * INV_LN2 + ((d >= 0.0) ? 0.5 : -0.5));
    r = (d - (double)k * LN2_HI) - (double)k * LN2_LO;
    p = 1.0 + r * (1.0 + r * (0.5 + r * (1.0 / 6.0 + r * (1.0 / 24.0 +
        r * (1.0 / 120.0 + r * (1.0 / 720.0 + r * (1.0 / 5040.0)))))));
    if (k < -1074)
        return 0.0f;
    return (float)(p * d_from_bits((unsigned long long)(k + 1023) << 52));
}

float vm_logf(float x)
{
    unsigned int u;
    int e;
    float mf;
    double m, s, s2, logm;
    memcpy(&u, &x, sizeof u);
    if (u & SIGN32)
        return nan32();
    if (u == 0u)
        return f_from_bits(0xff800000u);
    if ((u & EXPO32) == EXPO32)
        return (u & MANT32) ? x + x : x;
    e = (int)((u >> 23) & 0xffu) - 127;
    if (e == -127) {
        x *= 16777216.0f;
        memcpy(&u, &x, sizeof u);
        e = (int)((u >> 23) & 0xffu) - 127 - 24;
    }
    mf = f_from_bits((u & MANT32) | 0x3f800000u);
    m = (double)mf;
    if (m > SQRT2) {
        m *= 0.5;
        e += 1;
    }
    s = (m - 1.0) / (m + 1.0);
    s2 = s * s;
    logm = 2.0 * s * (1.0 + s2 * (1.0 / 3.0 + s2 * (1.0 / 5.0 + s2 *
        (1.0 / 7.0 + s2 * (1.0 / 9.0 + s2 * (1.0 / 11.0 + s2 / 13.0))))));
    return (float)((double)e * LN2_HI + ((double)e * LN2_LO + logm));
}

#define PIO2_HI 1.57079632679489661930e+00
#define PIO2_MID 6.12323399573676603587e-17
#define PIO2_LO -1.49738490485916980000e-33
#define TWO_OVER_PI 6.36619772367581382433e-01

/* x -> r in [-pi/4, pi/4], returns the quadrant 0..3 */
static int reduce_pi2(double x, double* r)
{
    double n = x * TWO_OVER_PI;
    long q = (long)(n + ((n >= 0.0) ? 0.5 : -0.5));
    *r = ((x - (double)q * PIO2_HI) - (double)q * PIO2_MID)
         - (double)q * PIO2_LO;
    return (int)(q & 3L);
}

static double sin_poly(double r)
{
    double z = r * r;
    return r * (1.0 + z * (-1.0 / 6.0 + z * (1.0 / 120.0 + z * (-1.0 / 5040.0
        + z * (1.0 / 362880.0 + z * (-1.0 / 39916800.0
        + z * (1.0 / 6227020800.0)))))));
}

static double cos_poly(double r)
{
    double z = r * r;
    return 1.0 + z * (-0.5 + z * (1.0 / 24.0 + z * (-1.0 / 720.0 +
        z * (1.0 / 40320.0 + z * (-1.0 / 3628800.0 +
        z * (1.0 / 479001600.0))))));
}

float vm_sinf(float x)
{
    double r;
    int q;
    unsigned int u;
    memcpy(&u, &x, sizeof u);
    if ((u & EXPO32) == EXPO32)
        return (u & MANT32) ? x + x : nan32();
    q = reduce_pi2((double)x, &r);
    switch (q) {
    case 0: return (float)sin_poly(r);
    case 1: return (float)cos_poly(r);
    case 2: return (float)-sin_poly(r);
    default: return (float)-cos_poly(r);
    }
}

float vm_cosf(float x)
{
    double r;
    int q;
    unsigned int u;
    memcpy(&u, &x, sizeof u);
    if ((u & EXPO32) == EXPO32)
        return (u & MANT32) ? x + x : nan32();
    q = reduce_pi2((double)x, &r);
    switch (q) {
    case 0: return (float)cos_poly(r);
    case 1: return (float)-sin_poly(r);
    case 2: return (float)-cos_poly(r);
    default: return (float)sin_poly(r);
    }
}
