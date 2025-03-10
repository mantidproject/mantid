#!/bin/bash -ex

# This script will set up a conda environment before running the cppcheck shell script.
#
# Script usage:
# conda-cppcheck <path-to-workspace>
#
# Example command to run a PR build on ubuntu:
# conda-cppcheck $WORKSPACE
#
# Expected args:
#   1. WORKSPACE: path to the workspace/source code that this should run inside, Windows Caveat: Only use / for
#                 this argument do not use \\ or \ in the path.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPT_DIR/mamba-utils

# Check 1 argument is passed, and is not optional.
if [[ $# < 1 || $1 == "--"* ]]; then
    echo "Pass 1 argument: conda-cppcheck <path-to-workspace>"
    exit 1
fi

WORKSPACE=$1
shift 1

cd $WORKSPACE

if $SCRIPT_DIR/../check_for_changes cpp; then
    echo "No C++ files have changed. Skipping check."
    exit 0
fi

# Setup conda environment
setup_mamba $WORKSPACE/miniforge
create_and_activate_mantid_developer_env $WORKSPACE

###############################################################################
# Run Cppcheck
###############################################################################

# If errors slip through to main this can be used to set a non-zero
# allowed count while those errors are dealt with. This avoids breaking all
# builds for all developers
ALLOWED_ERRORS_COUNT=0

# Create build directory if it doesn't exist
[ -d $WORKSPACE/build ] || mkdir $WORKSPACE/build
cd $WORKSPACE/build

# remove old results if they exist
find -name cppcheck.xml -delete

cmake --preset=cppcheck-ci -DCPPCHECK_NUM_THREADS=$BUILD_THREADS ..

# Run cppcheck
cmake --build . --target cppcheck

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
