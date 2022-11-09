#!/bin/bash -ex

SCRIPT_DIR=$(dirname "$0")
# If errors slip through to master this can be used to set a non-zero
# allowed count while those errors are dealt with. This avoids breaking all
# builds for all developers
ALLOWED_ERRORS_COUNT=0

###############################################################################
# Create the build directory if it doesn't exist
###############################################################################
[ -d $WORKSPACE/build ] || mkdir $WORKSPACE/build
cd $WORKSPACE/build

# remove old results if they exist
find -name cppcheck.xml -delete

cmake --version

cmake --preset=cppcheck-ci -DCPPCHECK_NUM_THREADS=$BUILD_THREADS ..

# run cppcheck
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
