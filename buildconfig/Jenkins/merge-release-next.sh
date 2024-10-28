#!/bin/sh

# Fetch and merge release-next into main

set -ex

git fetch origin --prune
git checkout main
git reset --hard origin/main

git config user.name mantid-builder
git config user.email "mantid-buildserver@mantidproject.org"

git merge origin/release-next
git push origin main
