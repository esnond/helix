#!/bin/sh
# push_run.sh - push the release branch and all phase tags to origin.
# Requires an HTTPS PAT at git's own credential prompt (user types it).
set -eu
cd "$(dirname "$0")/.."
branch=$(git branch --show-current)
echo "pushing $branch + tags to origin"
git push origin "$branch"
for t in r2-anchor r3-pre r3 r5 v1.3 v1.0-rc v1.0; do
    if git rev-parse -q --verify "refs/tags/$t" >/dev/null 2>&1; then
        git push origin "$t"
    fi
done
echo "done"
