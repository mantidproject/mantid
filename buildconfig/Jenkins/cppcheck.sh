#!/bin/bash -ex

SCRIPT_DIR=$(dirname "$0")
# If errors slip through to master this can be used to set a non-zero
# allowed count while those errors are dealt with. This avoids breaking all
# builds for all developers
ALLOWED_ERRORS_COUNT=0

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
if [ $(command -v scl) ]; then
    CMAKE_EXE=cmake3
    SCL_ENABLE="scl enable devtoolset-7"
else
    CMAKE_EXE=cmake
    SCL_ENABLE="eval"
fi
$SCL_ENABLE "$CMAKE_EXE --version"

if [ "$(command -v ninja)" ]; then
  CMAKE_GENERATOR="-G Ninja"
elif [ "$(command -v ninja-build)" ]; then
  CMAKE_GENERATOR="-G Ninja"
fi
$SCL_ENABLE "$CMAKE_EXE ${CMAKE_GENERATOR} -DCMAKE_BUILD_TYPE=Debug -DCPPCHECK_GENERATE_XML=TRUE -DCPPCHECK_NUM_THREADS=$BUILD_THREADS .."

# run cppcheck
$CMAKE_EXE --build . --target cppcheck

# Generate HTML report
cppcheck-htmlreport --file=cppcheck.xml --title=Embedded --report-dir=cppcheck-report

# Mark build as passed or failed. The additional "|| true" stops the build from failing if there are no errors.
errors_count=$(grep -c '</error>' cppcheck.xml) || true
if [ $errors_count -ne ${ALLOWED_ERRORS_COUNT} ]; then
  echo "CppCheck found ${errors_count} errors."
  echo "See CppCheck link on the job page for more detail, or adjust the count."
  exit 1
else
  echo "CppCheck found no errors"
  exit 0
fi
