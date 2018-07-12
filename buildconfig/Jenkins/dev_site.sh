#!/bin/bash -ex
if [ -z "$BUILD_DIR" ]; then
 if [ -z "$WORKSPACE" ]; then
     echo "WORKSPACE not set. Cannot continue"
     exit 1
 fi

 BUILD_DIR=$WORKSPACE/build
 echo "Setting BUILD_DIR to $BUILD_DIR"
fi

if [ -d $BUILD_DIR ]; then
  echo "$BUILD_DIR exists"
else
  mkdir $BUILD_DIR
fi

###############################################################################
# Print out the versions of things we are using
###############################################################################
# we use cmake3 on rhel because cmake is too old
if [ $(command -v cmake3) ]; then
    CMAKE_EXE=cmake3
else
    CMAKE_EXE=cmake
fi
${CMAKE_EXE} --version

###############################################################################
# Generator
###############################################################################
if [ "$(command -v ninja)" ]; then
  CMAKE_GENERATOR="-G Ninja"
elif [ "$(command -v ninja-build)" ]; then
  CMAKE_GENERATOR="-G Ninja"
fi
##### set up the build directory
cd $BUILD_DIR
if [ -e $BUILD_DIR/CMakeCache.txt ]; then
  ${CMAKE_EXE} .
else
  ${CMAKE_EXE} ${CMAKE_GENERATOR} ..
fi

if [ -d dev-docs/html ]; then
  echo "Updating existing checkout"
  cd dev-docs/html
  git pull --rebase
  cd -
else
  echo "Cloning developer site"
  git clone git@github.com:mantidproject/developer.git dev-docs/html || exit -1
  cd dev-docs/html
  git checkout gh-pages
  cd -
fi

##### build the developer site
${CMAKE_EXE} --build . --target dev-docs-html

cd dev-docs/html

if [ "builder" == "$USER" ]; then
    echo "Setting username"
    git config user.name mantid-builder
    git config user.email "mantid-buildserver@mantidproject.org"
fi

##### push the results
if [ $(git diff --quiet) ]; then
    echo "Committing new site"
    git add .
    git commit -m "Automatic update of developer site"
    git push
else
    echo "Nothing has changed"
fi
