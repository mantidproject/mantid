#!/bin/bash -ex

# Update the https://github.com/mantidproject/conda-recipes repository with the latest changes from the currently
# checked out branch of mantid.
#
# Expected args:
#   1. GITHUB_ACCESS_TOKEN: The token required to push to a https cloned mantidproject/conda-recipes repository
#   2. GITHUB_USER_NAME: The user name of the mantidproject/conda-recipes repository for committing and pushing
#   3. GITHUB_USER_EMAIL: The email address associated with the user
#
# Possible flags:
#   --local-only: update a local copy of the conda-recipes repository only without cloning, and do not push the changes
#     to github.
#   --release-version: generate a version number for a release candidate, using the patch version rather than the date.

GITHUB_ACCESS_TOKEN=$1
shift
GITHUB_USER_NAME=$1
shift
GITHUB_USER_EMAIL=$1
shift

# Optional flag to update a local repo without pushing the changes
LOCAL_ONLY=false
RELEASE_VERSION=false
RECIPES_TAG=main

# Handle flag inputs
while [ ! $# -eq 0 ]
do
    case "$1" in
        --local-only) LOCAL_ONLY=true ;;
        --release-version) RELEASE_VERSION=true ;;
        --recipes-tag)
            RECIPES_TAG="$2"
            shift ;;
        *)
            echo "Argument not accepted: $1"
            exit 1
            ;;
  esac
  shift
done

# Generate the version number
MAJOR_VERSION=$(command perl -n -e '/set\(VERSION_MAJOR\s+(\d+)\)/ && print $1' < buildconfig/CMake/VersionNumber.cmake)
echo $MAJOR_VERSION
MINOR_VERSION=$(command perl -n -e '/set\(VERSION_MINOR\s+(\d+)\)/ && print $1' < buildconfig/CMake/VersionNumber.cmake)
echo $MINOR_VERSION
# Use patch version for release packages, otherwise use date and time of the latest git sha
if [[ $RELEASE_VERSION  == true ]]; then
    PATCH_VERSION=$(command perl -n -e '/set\(VERSION_PATCH\s+(\d+)\)/ && print $1' < buildconfig/CMake/VersionNumber.cmake)
else
    LATEST_GIT_SHA_DATE=$(command git log -1 --format=format:%ci)
    echo $LATEST_GIT_SHA_DATE
    YEAR=${LATEST_GIT_SHA_DATE:0:4}
    MONTH=${LATEST_GIT_SHA_DATE:5:2}
    DAY=${LATEST_GIT_SHA_DATE:8:2}
    HOUR=${LATEST_GIT_SHA_DATE:11:2}
    MINS=${LATEST_GIT_SHA_DATE:14:2}
    PATCH_VERSION=$YEAR$MONTH$DAY.$HOUR$MINS
fi
echo $PATCH_VERSION

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
if [[ $LOCAL_ONLY  == false ]]; then
    if [ -d "conda-recipes" ]; then rm -rf conda-recipes; fi
    git clone https://${GITHUB_USER_NAME}:${GITHUB_ACCESS_TOKEN}@github.com/mantidproject/conda-recipes.git -b $RECIPES_TAG
fi

cd conda-recipes

function replace_version_data() {
    sed -ie 's/{% set git_commit =.*/{% set git_commit = "'$LATEST_GIT_SHA'" %}/' $1
    sed -ie 's/{% set version =.*/{% set version = "'$VERSION'" %}/' $1
    sed -ie 's/  sha256: .*/  sha256: '$SHA256'/' $1
}

replace_version_data recipes/mantid/meta.yaml
replace_version_data recipes/mantidqt/meta.yaml
replace_version_data recipes/mantiddocs/meta.yaml
replace_version_data recipes/mantidworkbench/meta.yaml

if [[ $LOCAL_ONLY  == false ]]; then
    if ! git diff --quiet --exit-code; then
        git config user.name ${GITHUB_USER_NAME}
        git config user.email ${GITHUB_USER_EMAIL}
        git add recipes/*/meta.yaml
        git commit -m "Update version and git sha" --no-verify --signoff
        git pull --ff
        git push
    else
        echo "No changes to commit"
    fi
fi
