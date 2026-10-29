# ROLLBACK — helix v10-run

## Anchor
- Branch: `v10-run` (branched from `main` at the v1.3 release commit)
- Tag: `v1.3-anchor` points to the last v1.3 commit before this run
- Baseline: green gates at anchor (make test + test-san + gate-libm +
  cargo test, see docs/STATE.md)

## Undo a phase
```bash
# revert all commits since anchor, keep working tree
git reset --soft r2-anchor

# revert all commits and discard changes
git reset --hard r2-anchor

# revert a single phase tag
git revert <tag>..HEAD
```

## Undo a single file
```bash
git checkout r2-anchor -- path/to/file
```

## Restore original branch
```bash
git checkout main
git branch -D v10-run      # only if run is abandoned
git tag -d v1.3-anchor v1.0-rc
```

## Environment reset
- Binary: `make clean && make`
- Rust: `cd helix-gui && cargo clean && cargo build`
- Keychain: `rm -f $HOME/.helix-kch-*.db`
- Results: `rm -rf results/phase*`

## Verify rollback
```bash
make test                    # must be green (14 suites)
make gate-libm               # must report no libm boundary
cd helix-gui && cargo test   # must be 7/7
```