#!/bin/bash -ex

SCRIPT_DIR=$(dirname "$0")

if [[ ${JOB_NAME} == *pull_requests* ]]; then
    # This relies on the fact pull requests use pull/$PR-NAME
    # which squashes the branch into a single merge commit
    cd $WORKSPACE

    if ${SCRIPT_DIR}/check_for_changes cpp; then
        echo "No C++ files have changed. Skipping check."
        exit 0
    fi
fi

###############################################################################
# Create the build directory if it doesn't exist
###############################################################################
[ -d $WORKSPACE/build ] || mkdir $WORKSPACE/build
cd $WORKSPACE/build

# remove old results if they exist
find -name cppcheck.xml -delete

# configure cmake
if [ "$(command -v ninja)" ]; then
  CMAKE_GENERATOR="-G Ninja"
elif [ "$(command -v ninja-build)" ]; then
  CMAKE_GENERATOR="-G Ninja"
fi
cmake ${CMAKE_GENERATOR} -DCMAKE_BUILD_TYPE=Debug -DCPPCHECK_GENERATE_XML=TRUE -DCPPCHECK_NUM_THREADS=$BUILD_THREADS ..

# run cppcheck
cmake --build . --target cppcheck
