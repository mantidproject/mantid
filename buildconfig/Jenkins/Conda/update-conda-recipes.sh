#!/bin/bash -ex

# Update the https://github.com/mantidproject/conda-recipes repository with the latest changes from the currently
# checked out branch.
#
# Expected args:
#   1. GITHUB_ACCESS_TOKEN: The token required to push to a https cloned mantidproject/conda-recipes repository
#   2. GITHUB_USER_NAME: The user name of the mantidproject/conda-recipes repository for committing and pushing

GITHUB_ACCESS_TOKEN=$1
GITHUB_USER_NAME=$2

# Checkout correct branch
git checkout release-next

# Generate the latest version number
LATEST_GIT_SHA_DATE=$(command git log -1 --format=format:%ci)
echo $LATEST_GIT_SHA_DATE
YEAR=${LATEST_GIT_SHA_DATE:0:4}
MONTH=${LATEST_GIT_SHA_DATE:5:2}
DAY=${LATEST_GIT_SHA_DATE:8:2}
HOUR=${LATEST_GIT_SHA_DATE:11:2}
MINS=${LATEST_GIT_SHA_DATE:14:2}
PATCH_VERSION=$YEAR$MONTH$DAY.$HOUR$MINS
echo $PATCH_VERSION
VERSION_MAJOR_STRING=$(command grep "set(VERSION_MAJOR" ./buildconfig/CMake/VersionNumber.cmake)
MAJOR_VERSION=${VERSION_MAJOR_STRING:20:1} # Only valid for 1 digit
echo $MAJOR_VERSION
VERSION_MINOR_STRING=$(command grep "set(VERSION_MINOR" ./buildconfig/CMake/VersionNumber.cmake)
MINOR_VERSION=${VERSION_MINOR_STRING:20:1} # Or a Minor version number?
echo $MINOR_VERSION
VERSION=$MAJOR_VERSION.$MINOR_VERSION.$PATCH_VERSION
echo $VERSION

# Get the latest git sha from the current repo
LATEST_GIT_SHA=$(command git log -1 --format=format:%H)
echo $LATEST_GIT_SHA

# Download a copy of the expected sha and generate a sha256 based on this download
SOURCE_FILE=$LATEST_GIT_SHA.tar.gz
URL=https://github.com/mantidproject/mantid/archive/$SOURCE_FILE
if [ -x "$(which curl)" ]; then
    curl -L -O $URL
elif [ -x "$(which wget)" ] ; then
    wget $URL
else
    echo "Could not download Conda as wget and curl are not installed."
    exit 1
fi
SHA_RESULT=$(command openssl sha256 $SOURCE_FILE)
STRING_ARRAY=($SHA_RESULT)
SHA256=${STRING_ARRAY[1]}
echo $SHA256
rm -rf $SOURCE_FILE

# Clone conda-recipes
if [ -d "conda-recipes" ]; then rm -rf conda-recipes; fi
git clone https://${GITHUB_USER_NAME}:${GITHUB_ACCESS_TOKEN}@github.com/mantidproject/conda-recipes.git

cd conda-recipes

function input_data(){
    sed -i '/{% set git_commit =/c\{% set git_commit = "'$LATEST_GIT_SHA'" %}' $1
    sed -i '/{% set version =/c\{% set version = "'$VERSION'" %}' $1
    sed -i '/  sha256: /c\  sha256: '$SHA256'' $1
}

input_data recipes/mantid/meta.yaml
input_data recipes/mantidqt/meta.yaml
input_data recipes/mantidworkbench/meta.yaml

git config user.name ${GITHUB_USER_NAME}
git config user.email ${GITHUB_USER_NAME}@mantidproject.org
git add recipes/*/meta.yaml
git commit -m "Update version and git sha" --no-verify --signoff
git pull --ff

git push