#!/bin/sh

# Fetch and merge release-next into master

set -ex

git fetch origin --prune
git checkout master
git reset --hard origin/master

git config user.name mantid-builder
git config user.email "mantid-buildserver@mantidproject.org"

git merge origin/release-next
git push origin master

