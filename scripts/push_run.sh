#!/bin/sh
# push_run.sh - push r3-r5-run branch and phase tags to origin.
# Requires an HTTPS PAT at git's own credential prompt (user types it).
set -eu
cd "$(dirname "$0")/.."
branch=$(git branch --show-current)
echo "pushing $branch + tags to origin"
git push origin "$branch"
git push origin r2-anchor
for t in r3-pre r3 r5 v1.3; do
    if git rev-parse -q --verify "refs/tags/$t" >/dev/null 2>&1; then
        git push origin "$t"
    fi
done
echo "done"
