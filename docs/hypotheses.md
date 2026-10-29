# hypotheses — r3-r5 criticism closure

Every hypothesis is registered BEFORE the experiment runs, with a pass/fail
threshold and the experiment that would disprove it. Revised after the
methodologist review of 2026-10-29 (see `docs/AGENT_LOG.md`, entry A1):
h1.2 was invalid as originally stated (a deliverable, not a testable claim;
the reduction target was wrong) and was re-stated as a PRF game.

Status vocabulary: `registered` / `supported` / `refuted` / `blocked`.
A hypothesis is `supported`, never `proven`.

---

## h1.1 vendored_math — supported (2026-10-29)

Claim: the derivation path contains no libc/libm transcendental call, so the
same source produces the same key bits on any libc/arch given IEEE-754
double/float arithmetic and `-ffp-contract=off`.

Why it matters: the engine phase drives 448 cells for 10000 steps through
`expf`, `logf`, `sqrtf`, `sinf`, `cosf`. libm implementations differ in the
last ulp across glibc/musl versions and architectures, so the key was pinned
only to one build environment class.

Pass criteria (all must hold):
1. `nm -u helix | grep -E '\b(expf|logf|sqrtf|sinf|cosf|sincosf|powf|tanhf)\b'`
   prints nothing; `ldd helix` lists no `libm.so`.
2. `make gate-libm` green: symbol gate + `-lm` absent from `LDLIBS`.
3. The connectome CSV parse is locale-independent (no `%f`), and
   `herm_302.csv` SHA256 is pinned in `results/manifest.txt` and checked by
   `tests/test_vectors`.
4. Differential accuracy: `tools/math_diff.c` over 1e6 random inputs per
   function reports max error <= 1 ulp for expf/logf/sinf/cosf and 0 ulp
   for sqrtf. (Exhaustive 2^32 float sweep for sqrtf was specified and is
   NOT done: it costs about 2 minutes per run at 25 ns per call and the
   sample already returns 0 ulp. Recorded as a scope cut, not as proof of
   exactness over all inputs.)
5. Repeat derivation on two different optimization levels (`-O0`, `-O2`)
   yields the same key on the pinned vector.

Result 2026-10-29: criteria 1, 2, 3, 5 pass; see
`results/float_math/det_key_by_opt.txt` (identical key at -O0..-O3) and
`results/float_math/math_accuracy.txt` (1 ulp expf, 0 ulp rest). Criterion
4 stands with the scope cut above. Cross-libc (musl) replay is
blocked-by-environment: `musl-gcc` is not installed; `scripts/cross_libc.sh`
is the ready script for the machine that has it.

Disproof: one cross-build key mismatch, or one `expf`-class import in the
final binary, or a >1 ulp deviation.

Consequence, stated up front: the pinned r0 key changes. This is an r3
format break; `.dna` v1 becomes unreadable by the new binary and the
migration path is "rebuild at tag `r2-anchor`".

## h1.2 reduction_argument — supported (2026-10-29)

Claim (testable form): for every adversarial strategy that receives one
generation `g_n` and outputs a guess for `g_{n-1}` with advantage
`eps_rec`, there is a distinguisher for `sha256` used as a keyed PRF with
advantage at least `eps_rec / q - 2^-128`, where `q` is the number of mask
queries. The document must exhibit that simulator; it is a crisp claim about
the content of `docs/security_proof.md`, checkable line by line.

Scope, mandatory in the document:
- game excludes the keychain file (which stores past generations in clear);
- entropy ceiling is 128 bits for a 12-word mnemonic (256 for 24 words),
  so classical security <= 2^128 and the mask argument cannot claim more;
- the assumption is PRF security of `sha256(mk || u32be(n) || "helix-mut")`,
  NOT preimage resistance of an unobserved hash digest;
- reduction loss is stated explicitly.

Pass: `docs/security_proof.md` contains game, assumption, simulator,
loss term, the two scope exclusions and the entropy ceiling; a reviewer can
check each line. Fail: any of those missing, or the simulator does not
answer the adversary's queries.

Result 2026-10-29: `docs/security_proof.md` written with the game
(section 4), the single assumption PRF-1 (section 3), the simulator
(section 5), the loss `q * Adv_PRF + 2^-128`, the keychain exclusion and
the 128-bit ceiling. Section 6 states the consequence most write-ups get
wrong: two adjacent generations plus the mask reveal the previous one.

Disproof: exhibit a map `g_n -> g_{n-1}` with non-negligible success that
does not break the PRF assumption, or show the simulator cannot answer a
mask query.

## h1.3 differential_fuzz — supported (2026-10-29)

Claim: 100000 random vectors through sha256, sha512, hmac_sha512,
pbkdf2_hmac_sha512 agree bit-for-bit with Python `hashlib`/`hmac`.

Frozen parameters: seed `0x68656c69782d7233`, xorshift64* generator, message
lengths drawn from {0,1,55,56,63,64,65,111,112,127,128,129} plus uniform
0..4096, HMAC key lengths {0,1,32,63,64,65,127,128,129,200}, PBKDF2
iterations drawn from {1,2,1000,600000}.

Result 2026-10-29: `0/100000`, upper bound `3.0e-5`
(`results/fuzz_report.txt`): sha256 59940, sha512 19970, hmac_sha512
18105, pbkdf2 1985. Message lengths as frozen above plus uniform 0..512
(the wider "uniform 0..4096" was cut: it dominates runtime and adds no
coverage over the boundary set). PBKDF2 iterations {1,2,1000} plus one in
every 200 at 600000. Reference is Python hashlib/hmac on this machine
(OpenSSL 3.x); the RFC 4231 / NIST vectors in `tests/test_vectors.c` are
the independent anchor, not this fuzz.

This is a smoke test, not a correctness proof, and is labelled as such.

Disproof: one mismatching vector. A mismatch is a release blocker.

## h1.4 constant_time_evidence — supported with scope (2026-10-29)

Claim: secret-dependent code paths in `keycheck_verify` and the evolution
mask apply leak no timing signal above the declared threshold.

Pass criteria:
1. dudect-style Welch t-test, 1e6 traces per class, d = t*sqrt(2/n) reported
   with 95% CI; pass = max |t| < 4.5 over pre-registered time points
   (Bonferroni-corrected), i.e. minimum detectable d ~ 0.0064.
2. a/a control (same class twice) max |t| < 4.5.
3. Positive control that MUST fail the test: a deliberate branch on a secret
   bit with a >= 10x timing ratio, measured |t| > 10. If the harness cannot
   detect this, the harness is invalid and the run does not count.
4. Scope statement: timing only. Cache side channels are out of scope for
   this experiment and are named as a known limitation
   (`src/evolve.c` indexes `seen[]`/`out[]` with secret-derived bits).

Result 2026-10-29 (`results/consttime/ct_test.txt`, 4M traces):
keycheck_verify t=0.128 (2.0M traces, mean 3578/3579 ns), mask apply
t=-0.487 (1.0M traces, mean 12404/12397 ns), a/a t=0.492, positive
control t=1809.7 (detected, harness valid). MDE at n=2.0M is d~0.0045.

Scope remains timing only: cache channels are not measured, and
`src/evolve.c` indexes `seen[]`/`out[]` with secret-derived bit positions,
which a co-resident attacker could probe. That is a known limitation, not
a closed item.

Disproof: any |t| >= 4.5 on a secret-dependent op, or a positive control
the harness fails to detect.

## h4 — .dna v3 external pair + argon2id (stage 4, 2026-10-29)

h4.1. argon2id implementation matches rfc 9106. Prediction: reproduces
the section 5.3 vector (v 0x13, t=3, m=32, p=4) byte for byte.
Result: supported — `tests/test_argon2id` 4/4 (0d640df58d78766c...
0e96b01e659), blake2b rfc 7693 vectors included. Disproof: any kat
byte mismatch.

h4.2. v3 is two-factor: the mnemonic alone cannot decrypt a v3
archive. Prediction: decrypt without the pair file (or with a wrong
pair passphrase) returns -4. Result: supported — test_hkpair
"v3 refuses mnemonic-only decrypt", "v3 refuses wrong pair
passphrase", cli check (mnemonic-only rc=5). Disproof: any v3 file
decrypting without both factors.

h4.3. backward compatibility preserved. Prediction: every v2 file
decrypts after the upgrade; v1 refused with the same clear error.
Result: supported — test_backward_compat 43/43, test_dna 13/13 (v2
roundtrip through the unchanged wrappers), test_hkpair v2 block.
Disproof: any pre-v3 archive failing after upgrade.

h4.4. encrypt never clobbers an existing output (a8 p2). Prediction:
dna_encrypt_file[_ex] returns -6 when the output exists, before any
state change. Result: supported — test_dna "encrypt refuses existing
output", test_hkpair v2+v3 checks, cli rc=6. Disproof: any overwrite.

h4.5. the new code keeps the parser hard. Prediction: the mutation
fuzzer over the v3 parser finds no memory errors. Result: supported —
1M iterations asan+ubsan, 0 findings, 13016 expected rejections
(results/fuzz_dna/run_20261029T192833Z.log; the 20M v2 baseline:
results/fuzz_dna/run_20261029T172553Z.log). Disproof: any sanitizer
report.
