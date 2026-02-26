#!/bin/bash -ex
# Install mantiddocs conda package from specified conda channel and publish to docs-nightly repo
# Required positional arguments:
# 1 - Conda channel to install mantiddocs from. This would normally be a local channel.
# 2 - Git SHA for the commit that was used to build the docs.
CONDA_CHANNEL=$1
GIT_SHA=$2

DOCS_GIT_REPOSITORY=https://github.com/mantidproject/docs-nightly
GIT_USER_NAME=mantid-builder
GIT_USER_EMAIL="mantid-buildserver@mantidproject.org"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT_DIR=$SCRIPT_DIR/../../..

# source util functions
. $SCRIPT_DIR/pixi-utils

# pixi
install_pixi

CONDA_PREFIX=$SCRIPT_DIR/docs_env

# Install freshly built mantid-docs from a local channel
pixi run --manifest-path $REPO_ROOT_DIR/pixi.toml -e docs-build mamba create -p $CONDA_PREFIX -c $CONDA_CHANNEL mantiddocs --yes

# Publish. Clone current docs to publish directory
# and rsync between installed html in html directory and current
# docs in publish. This will ensure documents are deleted in the
# published version if they are deleted in the build.
HTML_PATH=$CONDA_PREFIX/share/doc/html
git clone --depth 1 ${DOCS_GIT_REPOSITORY} publish
cd publish
pixi run --manifest-path $REPO_ROOT_DIR/pixi.toml -e docs-build rsync --archive --recursive --delete $HTML_PATH/* .
# Push
git config user.name ${GIT_USER_NAME}
git config user.email ${GIT_USER_EMAIL}
git add .
git commit -m "Publish nightly documentation from https://github.com/mantidproject/mantid/commit/${GIT_SHA}" || exit 0
git push https://${GITHUB_ACCESS_TOKEN}@github.com/mantidproject/docs-nightly main
