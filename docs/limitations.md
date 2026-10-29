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
4. cross platform replay: the libm boundary is gone (vendored math,
   gate-libm) and the key is identical at -O0..-O3. as of v1.0 the
   libc boundary (musl vs glibc) and the arm64 boundary (qemu-user
   replay, x86-64 decrypts arm64 output) are closed with evidence in
   results/cross_platform/. still unproven: macos, windows, big-endian
   targets.
5. the trajectory adds cost, not entropy. deterministic function of
   the master key (security_claims.md claim 5). any marketing that
   says otherwise is wrong, including earlier project copy.
6. keychain: v1.0 ships kch2 — the whole entry area is one
   chacha20poly1305 record under sha256(mk || "helix-kch") with a
   fresh nonce per rewrite, flock exclusivity, and in-place migration
   from kch1. residuals: the decrypted chain sits in ram for the
   handle's lifetime (scrubbed at close), and a torn 44-byte header
   write in a crash loses the chain (single-sector pwrite makes it
   unlikely, not impossible).
7. the derivation kdf is still pbkdf2-hmac-sha512 at 600k, not memory
   hard. argon2id (rfc 9106, kat-pinned) shipped in v1.0 but only
   protects .hkpair pair files (t=3, m=64 MiB); switching the master
   derivation itself would fork every existing keychain and is not
   done. design debt.
8. single user. the keychain now holds an exclusive flock for the
   handle's lifetime, but there is still no multi user separation and
   no per-entry access control.
9. zeroization is best effort. memset after use, no page locking, no
   mlock, the simulator holds derived state in a static buffer that
   is wiped but not locked.
10. /dev/urandom only for seed generation and for v3 encapsulation
    seeds. no hwrng fallback, no entropy health check beyond the
    kernel's.
11. a v3 archive decrypts only with the mnemonic AND the .hkpair
    file; losing the pair file loses those archives permanently. the
    pair passphrase is the second root secret and its entropy is the
    owner's responsibility.
12. the cli takes the pair passphrase via -p, which lands in shell
    history and the process list. the gui uses a password field; cli
    users should use a pager or a keyring wrapper.
13. a .dna decrypt writes the restored file over an existing path
    without asking (encrypt refuses; decrypt does not).
