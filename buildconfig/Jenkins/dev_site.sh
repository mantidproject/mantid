#!/bin/bash -ex

if [ -z "$BUILD_DIR" ]; then
 if [ -z "$WORKSPACE" ]; then
     echo "WORKSPACE not set. Cannot continue"
     exit 1
 fi

 BUILD_DIR=$WORKSPACE/build
 echo "Setting BUILD_DIR to $BUILD_DIR"
fi

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
# Setup virtualenv for building the docs
###############################################################################
VIRTUAL_ENV=$BUILD_DIR/venv
if [[  -d $VIRTUAL_ENV ]]; then
  rm -fr $VIRTUAL_ENV
fi
python3 -m venv $VIRTUAL_ENV
source $VIRTUAL_ENV/bin/activate
python3 -m pip install sphinx
python3 -m pip install sphinx_bootstrap_theme
which python3

###############################################################################
# Build the developer site
# -----------------------------------------------------------------------------
# the wacky long line is what is run from inside "sphinx-build" which is not
# installed by virtualenv for some reason
###############################################################################
python3 -m sphinx $WORKSPACE/dev-docs/source $BUILD_DIR

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
