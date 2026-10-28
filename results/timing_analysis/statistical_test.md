# h1 timing analysis

source: tools/timing_bench, interleaved measurement,
clock_gettime(CLOCK_MONOTONIC), ns per call.

## keycheck, matching tag (correct)

n = 20000, mean = 3260.3 ns, 95% CI [3249.1, 3271.4], sd = 805.5, p01 = 2998, p50 = 3015, p99 = 5077

## keycheck, mismatching tag (wrong)

n = 20000, mean = 3261.9 ns, 95% CI [3249.4, 3274.4], sd = 903.6, p01 = 2987, p50 = 3007, p99 = 5076

## keycheck, matching tag (a/a slot) (correct2)

n = 20000, mean = 3244.7 ns, 95% CI [3234.3, 3255.2], sd = 752.9, p01 = 2987, p50 = 3006, p99 = 4992

## positive control, equal bytes (control_fast)

n = 20000, mean = 23.9 ns, 95% CI [23.8, 24.1], sd = 9.7, p01 = 21, p50 = 22, p99 = 44

## positive control, byte 0 differs (control_slow)

n = 20000, mean = 23.7 ns, 95% CI [23.7, 23.8], sd = 6.2, p01 = 21, p50 = 22, p99 = 42

## correct vs wrong keycheck

mann whitney u = 249300425, z = 42.695, p = 0.0000
ks d = 0.26895 (crit 0.05 = 0.01360, crit 0.01 = 0.01630)
cohen's d = -0.0019

## a/a calibration (correct slot 1 vs correct slot 2)

mann whitney z = 45.945, p = 0.0000, ks d = 0.28645, cohen's d = 0.0199

## verdict

effect size correct-vs-wrong: cohen's d -0.0019, median delta 8 ns on a 3015 ns operation (0.27%); a/a harness bias: d 0.0199.
attributable beyond harness bias: no

exploitability (r1/r2): none demonstrated. keycheck is an offline operation - the caller sees the comparison result directly, so a timing oracle adds no information the return value does not already give. the residual 8 ns shift matters only for a network-exposed verifier, which does not exist until the r4 directory service; revisited there.

verdict: NO EXPLOITABLE TIMING ORACLE IN THE r1/r2 THREAT MODEL (constant time compare confirmed within 0.0199 d of harness noise at n=20000 per arm)

## positive control (must differ, validates the harness)

mann whitney z = -3.624, p = 2.90e-04, ks d = 0.30940, cohen's d = 0.025
