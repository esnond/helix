# h2 cache timing and power: measurement notes

## cache timing

no synthetic warm/cold csv: flushing caches to produce a cold
measurement requires privileged cache control (vm.drop_caches or
clflush on pinned lines) which this test environment does not grant.
instead the question was answered structurally: for a cache timing
leak there must be secret dependent CACHE STATE CHANGES. the audit
(memory_access_analysis.md) shows every secret dependent access in
helix lives inside one or two cache lines that are touched on every
call regardless of secrets, and the sha/hmac cores access memory
sequentially with public indices. with line granular access constant,
warm versus cold timing differences cannot correlate with secret
bytes.

## power patterns

requires physical measurement (oscilloscope, chipwhisperer class rig).
not performed, no data fabricated. the structural facts relevant to a
future measurement: the dominant power profile of a derivation is the
600000 iteration pbkdf2 (about 2 s of data independent hashing) plus
the 10000 step engine run (fixed work); the secret dependent portion
is a few microseconds of masked arithmetic hardened in this pass.

## conclusion

critical side channels found in helix-owned code: none. the hardening
applied (branchless fixed trip selection loop in evolve.c) removes the
one real branch on secret data. the remaining exposures (bip39 binary
search branches, vendor engine internals) are documented with the
threat model reasoning in memory_access_analysis.md and
docs/threat_model.md.
