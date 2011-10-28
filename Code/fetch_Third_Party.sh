#! /bin/bash
# This script deals with getting hold of the required third party includes and libraries
# It will either clone or pull 3rdpartyincludes & 3rdpartylibs-mac and put them in the right place for CMake

uname=`uname`
if [ ${uname} != 'Darwin' ]; then
  echo 'This script is only intended for use on Macs'
  echo 'Exiting without doing anything'
  exit 0
fi

gitcmd=`which git`
arch=mac64

if [ -z "${gitcmd}" ]; then
    echo 'Unable to find git, check that it is installed and on the PATH'
    exit 1
fi

function update {
    # Just making sure what we have is up to date
    echo Updating Third_Party includes and libraries...
    cd Third_Party/include
    ${gitcmd} pull
    cd ../lib/${arch}
    ${gitcmd} pull
    # Be sure to end up back where we started
    cd ../../..
    exit 0
}

function clone {
    echo Cloning Third_Party includes and libraries...
    # Find out the url where mantid came from so we use the same location & protocol
    url=`git config --get remote.origin.url | sed -e 's@/mantid.git@@'`
    echo "mantidproject URL set to ${url}"
    incs=${url}/3rdpartyincludes.git
    echo "URL for includes set to ${incs}"
    libs=${url}/3rdpartylibs-mac.git
    echo "URL for libraries set to ${libs}"
    ${gitcmd} clone ${incs} Third_Party/include
    ${gitcmd} clone ${libs} Third_Party/lib/mac64
    exit 0
}

# First check if everything is already there - if so we just want to update
if [ -d Third_Party/include ]; then
    update
else
    clone
fi


