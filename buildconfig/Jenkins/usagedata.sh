#!/bin/sh
# make testing easier - BUILD_DIR should be root of build tree
if [ -z "$BUILD_DIR" ]; then
    BUILD_DIR=`pwd`
    echo "setting BUILD_DIR to $BUILD_DIR"
fi

# remove old copy
cd $BUILD_DIR
if [ -f UsageData.zip ]; then
    echo "removing old version of UsageData.zip"
    rm UsageData.zip
fi

# create the symbolic link so the zip decompresses into the familiar name
cd ExternalData/Testing/Data
if [ -d UsageData ]; then
    echo "symbolic link already exists"
else
    echo "creating symbolic link"
    ln -s DocTest UsageData
fi

# remove the file if it exists - which shouldn't ever happen
if [ -f UsageData.zip ]; then
    echo "removing old version of UsageData.zip"
    rm UsageData.zip
fi

# create the zip archive
zip -r UsageData UsageData -x \*hash-stamp \*md5-stamp README\*
cd -
mv ExternalData/Testing/Data/UsageData.zip .
