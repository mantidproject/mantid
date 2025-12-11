# Steps:
#   - Install conda packages
#   - Build HTML docs
#   - Publish to GitHub pages
CONDA_LABEL=mantid/label/nightly
DOCS_GIT_REPOSITORY=https://github.com/mantidproject/docs-nightly
GIT_USER_NAME=mantid-builder
GIT_USER_EMAIL="mantid-buildserver@mantidproject.org"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATA_MIRROR="$("${SCRIPT_DIR}/data_mirrors")"

# source util functions
. source/buildconfig/Jenkins/Conda/pixi-utils

# pixi
install_pixi

# Configure a clean build directory
rm -rf build
mkdir build && cd build

# unset LD_PRELOAD as this causes cmake to segfault
LD_PRELOAD="" \
# Generate build files
pixi run -e docs-build --frozen cmake -G Ninja \
  -DDATA_STORE_MIRROR=${DATA_MIRROR} \
  -DMANTID_FRAMEWORK_LIB=SYSTEM \
  -DMANTID_QT_LIB=SYSTEM \
  -DENABLE_WORKBENCH=OFF \
  -DENABLE_PRECOMMIT=OFF \
  -DENABLE_DOCS=ON \
  -DDOCS_DOTDIAGRAMS=ON \
  -DDOCS_MATH_EXT=sphinx.ext.mathjax \
  -DDOCS_PLOTDIRECTIVE=ON \
  -DDOCS_SCREENSHOTS=ON \
  ${WORKSPACE}/source

# Configure the 'datasearch.directories' in the Mantid.properties file so the test data is found
# Docs should only require DocTestData which is a dependency of the target
export STANDARD_TEST_DATA_DIR=$PWD/ExternalData/Testing/Data
echo 'datasearch.directories = '$STANDARD_TEST_DATA_DIR'/DocTest/' >> $WORKSPACE/miniforge/envs/docs-build/bin/Mantid.properties

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
