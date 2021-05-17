#!/bin/sh
git checkout master
git pull --rebase

git config user.name mantid-builder
git config user.email "mantid-buildserver@mantidproject.org"

git merge origin/release-next
git push origin master

