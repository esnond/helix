# limitations

the short list a reviewer should read first.

1. crypto from scratch, unaudited. sha256, sha512, hmac, pbkdf2,
   bip39 and the chain logic are hand written c99. they are pinned to
   python hashlib/hmac stdlib output, rfc 4231 and the official
   bip-0039 vectors (about 1570 checks), but no third party has
   reviewed the code. hand written crypto is a red flag by default;
   the project's defense is honesty about it, not confidence.
2. one reduction, still no formal proof. claims 1-6 in
   security_claims.md are test enforced. the chain claim now has an
   explicit prf-game reduction in docs/security_proof.md with its loss
   term (q * Adv_PRF + 2^-128); that is a written argument a reviewer can
   check, not a mechanized proof, and the assumption is unproven.
3. side channels partially analyzed. timing measured with the h1.4
   harness: keycheck t=0.128 over 2.0m traces, mask apply t=-0.487 over
   1.0m, a/a 0.492, positive control 1809 (results/consttime/). cache
   side channels are NOT measured and src/evolve.c indexes seen[]/out[]
   with secret-derived bit positions; power and em unmeasured.
4. cross platform replay: the libm boundary is gone, the architecture
   boundary is not. the derivation path now carries its own math
   (src/vendored_math, no libm symbol in the binary, gate-libm), and the
   key is identical at -O0..-O3. unproven: another libc (musl-gcc is not
   installed; scripts/cross_libc.sh is ready), arm64, macos, windows.
5. the trajectory adds cost, not entropy. deterministic function of
   the master key (security_claims.md claim 5). any marketing that
   says otherwise is wrong, including earlier project copy.
6. keychain stores all past keys in plaintext. file permissions are
   the only defense. tamper evidence is not implemented (r2
   requirement, recorded in r1_security.md).
7. pbkdf2 is not memory hard. argon2id would resist gpu/asic better;
   pbkdf2 kept for r0 spec fidelity. design debt.
8. single user, single process. no keychain locking, no multi user
   separation.
9. zeroization is best effort. memset after use, no page locking, no
   mlock, the simulator holds derived state in a static buffer that
   is wiped but not locked.
10. /dev/urandom only for seed generation. no hwrng fallback, no
    entropy health check beyond the kernel's.
