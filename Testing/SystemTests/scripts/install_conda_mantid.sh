#!/usr/bin/env bash

# assumes that this is run with conda installed at /opt/conda
# and there is a conda environment  mantid-systemtests
# which has cmake and gcc installed

CONDA_PREFIX=/opt/conda

conda config --add channels conda-forge
conda config --add channels mantid
conda config --set always_yes true

# checking
source activate mantid-systemtests
echo "checking installation environment..."
pwd
which conda
conda env list
cat ~/.condarc

# make the tar ball available in local install channel
conda install conda
conda install conda-build
#
package=$1
VERSION=$(echo ${package} | sed -n 's/.*mantid-framework-\(.*\)-\(.*\)\.tar.bz2/\1/p')
BUILD=$(echo ${package} | sed -n 's/.*mantid-framework-\(.*\)-\(.*\)\.tar.bz2/\2/p')
mkdir -p ${CONDA_PREFIX}/conda-bld/linux-64
cp ${package} ${CONDA_PREFIX}/conda-bld/linux-64
set -e
conda index ${CONDA_PREFIX}/conda-bld
conda install -c ${CONDA_PREFIX}/conda-bld mantid-framework=${VERSION}=${BUILD}

# post-install check
echo "post-installation check ..."
conda list
PYTHON_EXE=`which python`
echo ${PYTHON_EXE}
${PYTHON_EXE} -c "import numpy"
${PYTHON_EXE} -c "import matplotlib; import mantid"
