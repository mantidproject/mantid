#!/bin/bash

# Creates a Debian package for the patched ParaView version. Generated for Ubuntu 10.04.
# 
# You do not need to run this script as sudo.


PV_LOC=paraview

PV_PKG=ParaView

PV_VER=3.98.1

PV_NAME=${PV_PKG}-${PV_VER}-source

PV_SRC=${PV_NAME}.tar.gz

NUM_PROCS=1


check_and_create_dir()

{

    if [ ! -d "$1" ]

    then

	mkdir ${1}

    fi

}


# Create directory for ParaView source

check_and_create_dir ${PV_LOC}



# Move tarball only if found in upper directory

if [ -f ${PV_SRC} ]

then

    mv ${PV_SRC} ${PV_LOC}

fi

cd ${PV_LOC}

if [ ! -d ${PV_NAME} ]

then

    tar zxvf ${PV_SRC}

fi



# Create build directory

check_and_create_dir "build"

cd build

# Ensure that the PV libaries end up in lib, and not some subdirectory there of
cmake_args="-DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=OFF -DPV_INSTALL_LIB_DIR=lib -DPARAVIEW_INSTALL_THIRD_PARTY_LIBRARIES=OFF"

cmake ${cmake_args} ../${PV_NAME}

make -j ${NUM_PROCS}

cd ../

check_and_create_dir "install"

cd install

echo "Starting Packaging"

cpack -D CPACK_DEBIAN_PACKAGE_DEPENDS=libqt4-dev -D CPACK_DEBIAN_PACKAGE_MAINTAINER="Owen Arnold, Martyn Gigg" -D CPACK_PACKAGING_INSTALL_PREFIX=/usr -G DEB --config ../build/Applications/ParaView/CPackParaView${dev_tag}Config.cmake

echo "Finished Packaging"

exit 0

