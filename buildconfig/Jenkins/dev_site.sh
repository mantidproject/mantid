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
  git clone git@github.com:mantidproject/developer.git $BUILD_DIR || exit -1
  cd $BUILD_DIR
  git checkout gh-pages
fi

###############################################################################
# Setup virtualenv for building the docs
###############################################################################
VIRTUAL_ENV=$BUILD_DIR/virtualenv
if [[ ! -d $VIRTUAL_ENV ]]; then
    virtualenv --system-site-packages "$VIRTUAL_ENV"
    source $VIRTUAL_ENV/bin/activate
    pip install sphinx
else
    source $VIRTUAL_ENV/bin/activate
fi
which python

###############################################################################
# Build the developer site
# -----------------------------------------------------------------------------
# the wacky long line is what is run from inside "sphinx-build" which is not
# installed by virtualenv for some reason
###############################################################################
SPHINX_VERS=$(python -c "import sphinx;print sphinx.__version__")
python -c "import sys;from pkg_resources import load_entry_point;sys.exit(load_entry_point('Sphinx==$SPHINX_VERS', 'console_scripts', 'sphinx-build')())" $WORKSPACE/dev-docs/source $BUILD_DIR

###############################################################################
# Push the results
###############################################################################
if [ "builder" == "$USER" ]; then
    echo "Setting username"
    git config user.name mantid-builder
    git config user.email "mantid-buildserver@mantidproject.org"
fi

if [ $(git diff --quiet) ]; then
    echo "Committing new site"
    git add .
    git commit -m "Automatic update of developer site"
    git push
else
    echo "Nothing has changed"
fi
