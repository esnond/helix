# ROLLBACK — helix r3-r5-run

## Anchor
- Branch start: `r3-r5-run` (branched from `main` at `de46886`)
- Tag: `r2-anchor` points to the last r2 commit before this run
- Baseline: `results/r2-baseline.txt` (all tests green, 2026-10-29)

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
git branch -D r3-r5-run    # only if run is abandoned
git tag -d r2-anchor
```

## Environment reset
- Binary: `make clean && make`
- Rust: `cd helix-gui && cargo clean && cargo build`
- Keychain: `rm -f $HOME/.helix-kch-*.db`
- Results: `rm -rf results/phase*`

## Verify rollback
```bash
make test          # must be green
cd helix-gui && cargo test   # must be 2/2
```