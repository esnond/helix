# publication strategy decision

chosen: option A, blog post. reasoning: the security pass found no
critical defects (no exploitable timing oracle, branchless hardening
applied, five config determinism matrix green), so option C is not
forced. but a workshop paper needs a provable contribution, and the
honest result (security_claims.md claim 5) is that the biological
trajectory adds computational cost and per generation diversity -
not strength. there is no reduction argument to review. a blog post
with full limitations is the right format; a paper becomes honest
only after cross platform replay is proven and, ideally, a strength
argument exists. draft below.

---

# Can a worm's nervous system derive your encryption keys? A research prototype

*what we tried, what worked, what did not, and what we refuse to claim*

this project started with a question that sounds like a joke: a c.
elegans connectome simulator is a fully deterministic dynamical
system - what happens if you use its trajectory as a key derivation
function? feed a bip39 seed into a 448 cell neural network, run it
for 10000 steps, hash the activations. helix is the result: about
3600 lines of c99, no external libraries, every primitive hand
written and vector crossed against python's stdlib.

**what worked.** the pipeline is bitwise deterministic: five compiler
configurations (-O0 through -O3 -march=native) produce the identical
32 byte key. the bip39 layer passes all 24 official vectors. the key
evolution chain - xor recombination of the two parent generations
plus a bounded 32..64 bit mutation - is a pure function of the master
key, so the whole key history is reproducible from the seed words
alone; the keychain is a cache, not a source of truth. timing
analysis with an a/a calibration harness found no exploitable oracle
in the offline operations, and the one branch on secret data we did
find got hardened to a branchless fixed trip loop with bit identical
output.

**what did not work, or worked differently than expected.** three
things. [v1.0 status: the libm item below was closed - the
derivation path now carries its own vendored math (gate-libm), and
musl + arm64 replays reproduce the key bit for bit. the other two
items stand.] first, the spec we wrote ourselves was wrong in ways only
implementation could reveal: timestamps in the mutation seed would
have destroyed cross machine replay; the bootstrap rule produced
identical gen_0 and gen_1; the expected hamming distribution between
consecutive generations (40-60 bits) was mathematically impossible
for xor recombination of pseudorandom keys - the measured
distribution is binomial(256, 1/2), mean 127.7, right where theory
says it must be. second, cross platform replay stops at the libm:
expf and sqrtf sit on the key path, and the c standard does not pin
transcendentals across libc versions. flags fix compiler level
nondeterminism; they cannot fix the library. the honest claim is
"bitwise reproducible within a build environment class", with key
fingerprints as the operational check. third, and most important:
the biological stage does not add security strength. it is a
deterministic function of the master key - zero new entropy, period.
what it adds is computational cost (about 1.2 s of simulation per
guess on top of 600000 pbkdf2 iterations) and an independent looking
256-bit projection per generation. that is a real property, but it
is not the property a casual reader assumes.

**why publish the negative result.** crypto from scratch is a red
flag, and reviewers were right to say so. the interesting artifact
here is not a new kdf - it is the methodology: every number in the
repo is a script plus a csv plus a seed, every claim names the
experiment that would disprove it, and the limitations file is the
first thing the readme points at. the prototype is useful as a
teaching object: how a determinism gate works, why a timing test
needs a calibration arm, how a spec dies on contact with
implementation.

**do not use this for real secrets.** use libsodium or age. the
repository says so in the first line, and now you know why in
detail: unaudited hand written crypto. the v1.0 run closed several of
the review gaps - secrets scrubbed with volatile memzero and mlock, a
20m-iteration asan/ubsan fuzz of the parser with zero findings, an
argon2id (rfc 9106, kat-pinned) protected external kyber pair as a
second factor, an encrypted keychain, and musl + arm64 replays with
identical keys - but an audit is still missing, and that item has no
self-serve fix.

code and full measurement set: [repository url]
