# h2 side channel audit: memory access patterns

method: static review of every secret-dependent memory access and
branch in helix code (vendor engine excluded, verbatim import, see
below), r1 code as of commit history through the h2 hardening.

## evolve.c - mutation position selection (hardened in this pass)

before: the selection loop skipped already chosen bit positions with a
data dependent branch and wrote `seen[]`/`out[]` under secret
dependent conditions. after hardening (this commit) the loop runs a
fixed 512 iterations with a branchless body: `take` and `apply` are
arithmetic masks, the trip count is public, the sha256 stream rehash
fires at public draw indices. output is bit for bit identical to the
early exit loop (verified: chain hamming statistics and all pinned
tests unchanged). residual secret dependent state: the values stored
in `seen[]`/`out[]`, which live in two adjacent cache lines that are
read and written on every call regardless - line level access pattern
is constant, so no cache timing signal at line granularity. residual
data dependent VALUES in registers: x86 has no value dependent
instruction latency for these ops.

## keycheck.c - constant time compare

volatile accumulate compare, no branches, no early exit. h1 measured
it: median delta 8 ns of ~3000 ns, below harness position bias
(results/timing_analysis/). no secret dependent memory access: the
compare reads both 32 byte tags linearly.

## sha256.c / sha512.c / hmac.c / pbkdf2.c

round constant tables (sha256_k.h, sha512_k.h) are indexed by the
round counter only - public, sequential, cache friendly. no s-box
style data indexed tables exist. message schedules and buffers are
accessed sequentially. pbkdf2 iteration count is a compile time
constant (600000); work does not depend on secret bytes. hmac block
handling branches only on the public key length.

## bip39.c - wordlist lookup (accepted limitation, documented)

mnemonic words are looked up by binary search: comparison count and
branches depend on the mnemonic itself, which is the master secret.
an attacker co located on the machine could in principle read word
index timing at ~11 bits per word granularity. accepted for r1/r2:
helix is an offline cli, the same attacker with local process
visibility has far cheaper attacks (read the process memory). revisit
if helix ever parses mnemonics as a service (r4).

## keychain.c

all entry access is at offsets derived from the generation number,
which is public metadata (it will be stored in the clear in the r2
.dna header by design). no secret dependent addressing.

## vendor engine (sim, neuron, body, world, modulators, plasticity)

verbatim import, untouched by helix. the engine's control flow is
key dependent through the rng stream, but its memory footprint
(connectome, cell array, body state) is fixed size and touched every
step regardless; there are no data indexed lookups on secret derived
indices. the whole trajectory runs for a fixed 10000 steps.

## what this audit cannot cover on this machine

- power analysis: requires oscilloscope or cpa capable hardware,
  out of scope, unmeasured (no synthetic csv is provided - none was
  measured).
- electromagnetic emanation: requires hardware, out of scope.
- microarchitectural attacks beyond cache timing (branch prediction
  port contention, prefetch side channels): no test harness available;
  the branchless hardening above removes the main secret dependent
  branches in helix code, the sha256/hmac cores are branch free on
  secret data by construction.
