/* deterministic scalar math for the derivation path.

   the engine phase of key derivation (10000 steps, 448 cells) used libm
   expf/logf/sqrtf/sinf/cosf. those are not bit-pinned across libc
   versions: glibc 2.27, glibc 2.41 and musl all differ in the last ulp,
   which is enough to change the derived key after 10000 steps.

   every function here is a fixed sequence of IEEE-754 float/double
   operations with no external call. combined with -ffp-contract=off the
   same source produces the same bits on any conforming platform, which
   is the property the key depends on (docs/hypotheses.md h1.1).

   accuracy against a correctly rounded reference is measured by
   tools/math_diff.c and published in results/math_accuracy.txt; the
   contract is <= 1 ulp for the transcendentals and exact for sqrt. */
#ifndef HELIX_VMATH_H
#define HELIX_VMATH_H

float vm_fabsf(float x);
double vm_fabs(double x);
float vm_sqrtf(float x);
float vm_hypotf(float x, float y);
double vm_hypot(double x, double y);
float vm_expf(float x);
float vm_logf(float x);
float vm_sinf(float x);
float vm_cosf(float x);

#endif
