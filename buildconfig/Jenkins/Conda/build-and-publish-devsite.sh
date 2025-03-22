#!/bin/bash -ex
# Build the developer site and push to gh-pages
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPT_DIR/mamba-utils

if [ $# != 1 ]; then
  echo "Usage: build-and-publish-devsite.sh mantid_root_dir"
  exit 1
fi

# Set working area
WORKSPACE=$1
BUILD_DIR=$WORKSPACE/build

###############################################################################
# Set up the build directory
###############################################################################
if [ -d $BUILD_DIR ]; then
  echo "$BUILD_DIR exists - updating existing checkout"
  cd $BUILD_DIR
  git pull --rebase
else
  echo "$BUILD_DIR does not exist - cloning developer site"
  git clone -b gh-pages https://github.com/mantidproject/developer.git $BUILD_DIR || exit -1
  cd $BUILD_DIR
fi

###############################################################################
# Mamba
###############################################################################
setup_mamba $WORKSPACE/miniforge "devsite"
mamba install --yes sphinx sphinx_bootstrap_theme

###############################################################################
# Build the developer site
###############################################################################
export LC_ALL=C
python -m sphinx $WORKSPACE/dev-docs/source $BUILD_DIR

###############################################################################
# Push the results
###############################################################################
echo "Setting username"
git config user.name mantid-builder
git config user.email "mantid-buildserver@mantidproject.org"
# commit returns true if nothing happened
git add .
git commit -m "Automatic update of developer site" || exit 0
git push https://${GITHUB_ACCESS_TOKEN}@github.com/mantidproject/developer gh-pages
