#!/bin/bash -ex
# Build the developer site and push to gh-pages

if [ $# != 1 ]; then
  echo "Usage: build-and-publish-devsite.sh mantid_root_dir"
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Set working area
WORKSPACE=$1
BUILD_DIR=$WORKSPACE/build
MAMBAFORGE_DIR=$WORKSPACE/mambaforge

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
# Setup environment for building the docs
###############################################################################
# false means do not install if it exists already
"$SCRIPT_DIR/download-and-install-mambaforge" $MAMBAFORGE_DIR $MAMBAFORGE_DIR/bin/mamba false
condaenv_prefix=$MAMBAFORGE_DIR/envs/dev-site-build
if [ -d "$condaenv_prefix" ]; then
  "$MAMBAFORGE_DIR/bin/mamba" update --prefix "$condaenv_prefix" --all
else
  "$MAMBAFORGE_DIR/bin/mamba" create --prefix "$condaenv_prefix" --yes sphinx sphinx_bootstrap_theme
fi
. $MAMBAFORGE_DIR/etc/profile.d/conda.sh
conda activate "$condaenv_prefix"

###############################################################################
# Build the developer site
###############################################################################
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
