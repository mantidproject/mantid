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

# we use cmake3 on rhel because cmake is too old
if [ $(command -v cmake3) ]; then
    CMAKE_EXE=cmake3
    CPACK_EXE=cpack3
    CTEST_EXE=ctest3
else
    CMAKE_EXE=cmake
    CPACK_EXE=cpack
    CTEST_EXE=ctest
fi
$CMAKE_EXE --version
${CMAKE_EXE} --build . --target StandardTestData

# create the symbolic link so the zip decompresses into the familiar name
pwd
cd ExternalData/Testing/Data || exit -1
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
