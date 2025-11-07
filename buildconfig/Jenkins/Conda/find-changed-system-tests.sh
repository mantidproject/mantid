#!/usr/bin/env bash
set -euo pipefail

BASE_BRANCH="${1:-main}"  # Default base branch if not provided

# Always operate from repo root
cd "$(git rev-parse --show-toplevel)"

# Find changed files
changed_files=$(git diff --name-only "${BASE_BRANCH}"...HEAD)

# Extract system test names
regex=$(
  echo "$changed_files" |
  grep '^Testing/SystemTests/tests/.*\.py$' |
  xargs -r -n1 basename |      # Keep only the filename
  sed -E 's/\.py$//' |         # Remove .py extension
  tr '\n' '|' |                # Join with |
  sed 's/|$//'                 # Remove trailing |
)

# Output result
if [[ -n "$regex" ]]; then
  echo "$regex"
else
  echo "No changed system test files found." >&2
fi
