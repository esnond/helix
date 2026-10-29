# STATE — helix r3-r5-run

Checkpoint file. Resume from here, never from zero.
Written 2026-10-29.

## Position

- phase: v1.0 run, stage 1 (gui rewrite) done
- branch: v10-run (anchor tag v1.3-anchor), next: stage 4 .dna v3 external kyber keypair + argon2id
- open item for stage 4: encrypt must refuse overwriting an existing .dna (A8 P2)
- branch: `r3-r5-run`
- anchor tag: `r2-anchor` (== main @ `de46886`)
- head at v1.3: 040f7b0 (tag v1.3); tags r2-anchor, r3, r5

## Phase 0 exit criteria

| criterion | verdict | evidence |
|---|---|---|
| branch created off r2 | done | `git checkout -b r3-r5-run` |
| anchor tag pushed | done-local / blocked-push | tag `r2-anchor` exists; push pending user PAT |
| baseline green | done | `results/r2-baseline.txt` |
| manifest generated | done | `results/manifest.txt` binary sha `ba25ad41…e941c` |
| v5 reference committed | done | `docs/design/v5_reference.html` |
| constitution committed | done | `docs/design/constitution.md` |
| rollback documented | done | `docs/ROLLBACK.md` |
| state checkpoint | done | this file |

## Items — full ledger

| id | item | phase | verdict | evidence | commit |
|---|---|---|---|---|---|
| p0.1 | branch + tag | 0 | done | `r2-anchor` | — |
| a2.1 | DoS: unauth generation drives 2^32 recombinations | 1 | done | DNA_GEN_MAX=4096 | (phase1 commit) |
| s1 | gui rewrite from scratch, v5 language, mouse-first | v1.0.1 | done | commit b320457, cargo test 7/7, gui_acceptance.md | b320457 |
| s1.a7 | cancel semantics: c call not interruptible | v1.0.1 | named | AGENT_LOG A7, checklist item 18 | 72e320e |
| s1.a8 | encrypt overwrites existing .dna silently | v1.0.1 | open | AGENT_LOG A8 | — |
| s2 | secure scrub: volatile memzero + mlock, 60+ call sites | v1.0.2 | done | tests/test_secure_mem 4 ok, suite 12/12 | 2f8abe8 |
| s3 | .dna parser fuzzer | v1.0.3 | done | 20M execs, 0 memory findings (results/fuzz_dna/run_20261029T172553Z.log) | 0e83d46+830e8aa |
| s4 | .dna v3 external pair + argon2id + A8 P2 overwrite refusal | v1.0.4 | done | rfc 9106 kat 4/4; 14/14 suites; asan+ubsan clean; gate-libm ok; test_hkpair 25/25; cargo 7/7 | 0743211 (core), ae67a27 (gui) |
| s5 | keychain kch2 (aead, flock, kch1 in-place migration) | v1.0.5 | done | test_keychain 64/64; 14/14 suites; asan+ubsan clean; gate-libm ok; cargo 7/7 | (stage5 commit) |
| s6 | cross-libc + arm64 replays | v1.0.6 | done | musl == glibc == aarch64(qemu), v2/v3 roundtrips, cross decrypt (results/cross_platform/) | (stage6 commit) |
| s7 | release v1.0-rc | v1.0-rc | done | r6_spec, limitations, claims 8-9, README, RELEASE_v1.0, manifest; tag v1.0-rc | 48155e8 |

## next

- owner: hands-on acceptance via docs/gui_acceptance.md (29 checks)
- after acceptance: tag v1.0, push via scripts/push_run.sh (owner types pat)
| p0.2 | baseline test + manifest | 0 | done | `results/` | — |
| p0.3 | v5 reference + constitution | 0 | done | `docs/design/` | (phase0 commit) |
| p0.4 | ROLLBACK.md | 0 | done | `docs/ROLLBACK.md` | (phase0 commit) |
| h1.1 | vendored_math, no libm, csv pin, -O0..-O3 identical | 1 | done | results/float_math/ | (phase1 commit) |
| h1.2 | PRF-game reduction | 1 | done | docs/security_proof.md | (phase1 commit) |
| h1.3 | differential fuzz 100k, 0 mismatch | 1 | done | results/fuzz_report.txt | (phase1b commit) |
| h1.4 | dudect t-test, targets < 4.5, pos control 1809 | 1 | done | results/consttime/ | (phase1b commit) |
| r3 | kyber-768 hybrid, .dna v2 kdf_type=1, kdf_type=0 decoded | 2 | done | tests/test_kyber 14 ok, docs/r3_spec.md | (r3 commit) |
| a4.1 | v1 files carried an all-zero salt | 2 | done | fixed in dna.c, verified | (r3 commit) |
| r4p | helixd cancelled, documented (HX-8) | 3 | done | docs/r4_decision.md | (r4p commit) |
| r5 | gui in v5 language: palette, layout, jobs, dendrite, i18n | 4 | done | docs/r5_spec.md, cargo test 2/2 | a0a3243 |
| rel | v1.3 release notes, README, limitations, claims | 5 | done | docs/RELEASE_v1.3.md | (release commit) |

## Push checklist (user-held PAT)

```
git push origin r3-r5-run
git push origin r2-anchor
```

## Environment notes

- repo: `~/helix` (not `~/petri`)
- engine: `~/elegans` v1.0.1 (vendored)
- `LD_PRELOAD=libstdbuf.so` exported in shell aborts ASAN binaries; `make test-san` clears it
- push needs HTTPS+PAT typed at git's own prompt; never through chat
