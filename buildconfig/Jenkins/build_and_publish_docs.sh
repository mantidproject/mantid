# Steps:
#   - Install conda packages
#   - Build HTML docs
#   - Publish to GitHub pages
CONDA_LABEL=mantid/label/nightly
DOCS_GIT_REPOSITORY=https://github.com/mantidproject/docs-nightly
GIT_USER_NAME=mantid-builder
GIT_USER_EMAIL="mantid-buildserver@mantidproject.org"

# source util functions
. source/buildconfig/Jenkins/Conda/mamba-utils

# Install conda and environment
setup_mamba $WORKSPACE/mambaforge "docs-build" true
mamba env update -n docs-build -f source/mantid-developer-linux.yml --prune
mamba install -c ${CONDA_LABEL} --yes mantidqt rsync

# Configure a clean build directory
rm -rf build
mkdir build && cd build

# Generate build files
cmake -G Ninja \
  -DMANTID_FRAMEWORK_LIB=SYSTEM \
  -DMANTID_QT_LIB=SYSTEM \
  -DENABLE_WORKBENCH=OFF \
  -DENABLE_PRECOMMIT=OFF \
  -DENABLE_DOCS=ON \
  -DDOCS_DOTDIAGRAMS=ON \
  -DDOCS_MATH_EXT=sphinx.ext.mathjax \
  -DDOCS_PLOTDIRECTIVE=ON \
  -DDOCS_QTHELP=OFF \
  -DDOCS_SCREENSHOTS=ON \
  ${WORKSPACE}/source

# Build the StandardTestData target. We need this test data to build docs-html
cmake --build . --target StandardTestData

# Configure the 'datasearch.directories' in the Mantid.properties file so the test data is found
export STANDARD_TEST_DATA_DIR=$PWD/ExternalData/Testing/Data
echo 'datasearch.directories = '$STANDARD_TEST_DATA_DIR'/UnitTest/;'$STANDARD_TEST_DATA_DIR'/DocTest/' >> $WORKSPACE/mambaforge/envs/docs-build/bin/Mantid.properties

# Build the html docs
export LC_ALL=C
QT_QPA_PLATFORM=offscreen cmake --build . --target docs-html

# Publish. Clone current docs to publish directory
# and rsync between built html in html directory and current
# docs in publish. This will ensure documents are deleted in the
# published version if they are deleted in the build.
cd docs
git clone --depth 1 ${DOCS_GIT_REPOSITORY} publish
cd publish
rsync --archive --recursive --delete ../html/* .
# Push
git config user.name ${GIT_USER_NAME}
git config user.email ${GIT_USER_EMAIL}
git add .
git commit -m "Publish nightly documentation" || exit 0
git push https://${GITHUB_ACCESS_TOKEN}@github.com/mantidproject/docs-nightly main

