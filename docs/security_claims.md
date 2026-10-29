# security claims

each claim states what proves or disproves it (falsifiability per the
methodology gate). a claim without a proof or a test is marked as
assumption.

## claim 1: same mnemonic + salt + build environment = same key, bitwise

proven. enforced on every test run: in process replay, cross process
cmp gate, pinned r0 value, five compiler configurations
(results/cross_platform/platform_results.csv). the experiment that
would disprove it: any suite run, anywhere, any time.

scope restriction, narrowed in r3: the libm boundary is closed. the key
path uses src/vendored_math (no expf/logf/sqrtf/sinf/cosf import;
`make gate-libm`), and the key is identical at -O0/-O1/-O2/-O3
(results/float_math/det_key_by_opt.txt). the connectome table is hashed
in tests and in results/manifest.txt. still an ASSUMPTION: the same bits
on another architecture or libc; musl replay is blocked by environment
(scripts/cross_libc.sh). the experiment that would disprove the wider
claim: build and run on arm64 or musl and compare the key.

## claim 2: evolution chain generations are pairwise distinct and deterministic

proven for the tested chains: 200 step and 50 step chains all
distinct (tests/test_evolution.c), two fresh keychains produce
identical sequences bitwise (tests/test_evolve.c, shell gate).
disproof experiment: any collision found in a chain run.

## claim 3: compromise of gen_n does not reveal gen_{n-1}

proven in the limited sense tested: gen_{n-1} is hidden behind a
256-bit mask that is computationally independent of gen_n
(tests/test_forward_secrecy.c: mask always applied, dictionary of
master key candidates never predicts). the proof rests on the
ASSUMPTION that sha256 is preimage resistant. the honest boundary:
this says nothing about the keychain file, which contains gen_{n-1}
in plaintext (docs/r1_security.md).

## claim 4: future generations are unpredictable from the keychain

same structure: proven modulo sha256 preimage resistance, in the
dictionary experiment. disproof: a prediction method that beats
brute forcing the master key material.

## claim 5: the biological trajectory adds security strength

FALSE as stated, documented as the project's central misconception
correction. the trajectory is a deterministic function of the master
key: it adds zero entropy. what it adds: (a) computational cost per
guess (about 1.2 s of simulation on top of the pbkdf2 cost), (b) an
independent looking 256-bit projection of the master key per
generation, (c) the research artifact itself. any publication claim
is limited to cost and diversity, never to strength. the experiment
that would change this verdict: a reduction argument showing the
trajectory stage hardens brute force beyond its wall clock cost -
nobody has one, including us.

## claim 6: timing side channels do not leak keycheck results

proven within measurement resolution: constant time compare, median
delta 8 ns of ~3000 ns, below harness position bias, n=20000 per arm
(results/timing_analysis/statistical_test.md). falsified-by design:
any future run of tools/timing_bench with a reproducible effect above
the a/a calibration. scope: offline operations only; r4 network
exposure requires new analysis.

## claim 7 (assumption, unproven): pbkdf2-hmac-sha512 at 600k costs ~2.8 s and resists gpu/asic speedup like standard pbkdf2

the implementation is vector crossed against python's hashlib, but
the cost ASSUMPTION inherits everything known about pbkdf2 (memory
hardness: none; asic parallelization: moderate). argon2id would be
the modern choice; pbkdf2 was chosen for r0 spec fidelity. recorded
as a design debt, revisit before any production claim.
