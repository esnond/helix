# limitations

the short list a reviewer should read first.

1. crypto from scratch, unaudited. sha256, sha512, hmac, pbkdf2,
   bip39 and the chain logic are hand written c99. they are pinned to
   python hashlib/hmac stdlib output, rfc 4231 and the official
   bip-0039 vectors (about 1570 checks), but no third party has
   reviewed the code. hand written crypto is a red flag by default;
   the project's defense is honesty about it, not confidence.
2. no formal security proofs. claims 1-6 in security_claims.md are
   test enforced, not proven. the mask hiding arguments reduce to
   sha256 preimage resistance as an assumption.
3. side channels partially analyzed. timing measured (n=20000, with
   a/a calibration and a positive control); memory access reviewed
   per module and hardened where cheap; power and em unmeasured (no
   hardware); microarchitectural attacks (prefetch, port contention)
   unmeasured.
4. cross platform replay unproven. proven: five compiler
   configurations on x86_64. unproven: arm64, macos, windows, other
   libc versions. the libm boundary (expf/sqrtf/sinf/cosf on the key
   path) is structural, not fixable by flags. mitigations:
   fingerprint verification, r2 header space reserved.
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
