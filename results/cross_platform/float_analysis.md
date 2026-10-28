# h3 float determinism analysis

## where floating point enters the derived key

every step's cell activations are hashed (r0 step 5), so the full
float trajectory is on the key path. float operations in the derive
path, all binary32 (ieee 754 single precision):

- neuron.c: graded membrane integration (semi implicit euler), sigmoid
  activation `neuron_act` (expf), membrane noise (sqrtf per step)
- body.c: force balance, nmj weights (expf)
- world.c: odor field setup and diffusion (expf, sinf, cosf)
- helix: quantization `(uint8_t)(act * 255.0f + 0.5f)`

## what determinism requires, and what enforces it

1. no fma contraction: `-ffp-contract=off` in every build (also in the
   vendored engine's own makefile) - verified by the o0/o1/o2/o3/native
   matrix below producing identical keys.
2. no reassociation or approximations: no -ffast-math/-Ofast anywhere
   in the repo (grep enforced by the determinism gate).
3. no excess precision: binary32 ops must round to binary32 at each
   step. on x86_64 this is automatic (sse2). on i386 the default x87
   fpu computes in 80 bit - that is the documented boundary, see below.
4. identical libm results: `expf`, `sqrtf`, `sinf`, `cosf` are on the
   key path. this is the sharpest edge of the whole design: the c
   standard does not require bitwise identical transcendental results
   across libc implementations or versions.

## matrix results (this machine, gcc 14.2, glibc 2.41, debian 13)

see platform_results.csv: x86_64 at -O0, -O1, -O2, -O3 and
-O3 -march=native all produce the identical key
4b6d7dcd2c168ad86b7274f2bf5cb902141d9983493d6d1267bff08526a9f8b2 for
the fixed test mnemonic + salt. five code generation strategies, one
answer - compiler level nondeterminism is ruled out.

## the i386 boundary (blocked build on this machine)

i386 configurations require libc6-dev-i386, not installed here and
not installable without root (sudo apt install gcc-multilib
libc6-dev-i386). predicted outcomes, to be verified when available:

- i386 -mfpmath=sse -msse2: expected MATCH (same sse binary32
  rounding, possibly different libm ifunc variants - the empirical
  test matters)
- i386 default x87: expected MISMATCH (80 bit intermediates change
  rounding of the membrane integration)
- i386 x87 -fexcess-precision=standard: possible match, must be
  measured, not assumed

if x87 mismatches, the platform requirement is precise: any build
must use ieee binary32 arithmetic for the engine (sse2, neon, or
x87 with enforced excess precision), and a keychain moving between
machines must be verified against a recorded key fingerprint before
use (procedure in README).

## the libm boundary (unresolvable by flags, documented as a limit)

flags cannot pin `expf` across glibc versions or across libc's.
within one libm build the results are reproducible; across libm
versions they may differ by 1 ulp, which the trajectory can amplify.
this is an honest structural limit of using a float based simulator
as a kdf: bitwise replay is a property of a build environment class,
not of the algorithm. mitigations, in order of honesty:

1. record the derivation fingerprint (first 8 bytes of sha256 of the
   key) with every keychain and every .dna file (r2 header space is
   already reserved in the brief's design)
2. on mismatch, refuse and report rather than silently derive a wrong
   key
3. long term option, not scheduled: replace the engine's expf/sqrtf
   call sites with a vendored, spec-pinned binary32 implementation -
   changes vendor files, needs its own validation milestone
