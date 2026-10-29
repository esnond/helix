# STATE — helix r3-r5-run

Checkpoint file. Resume from here, never from zero.
Written 2026-10-29.

## Position

- phase: 4 done (r5 gui), phase 5 in progress (release v1.3)
- next: final gate, manifest, tag v1.3, push by owner
- branch: `r3-r5-run`
- anchor tag: `r2-anchor` (== main @ `de46886`)
- head at phase 0 close: (this commit)

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
