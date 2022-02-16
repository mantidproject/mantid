#!/bin/bash

# Delete all but the latest nightly build
#
# Expected args:
#   1. WORKSPACE: path to the workspace/source code that this should run inside
#   2. ANACONDA_TOKEN: The token required to interact with the mantid channel on anaconda.org
#
# Optional args:
#   --channel: The channel to remove from (defaults to mantid)
#   --package: The package to remove from the channel (defaults to mantid)
#   --label: The label to remove from the channel (defaults to nightly)
WORKSPACE=$1
shift
ANACONDA_TOKEN=$1
shift

CHANNEL=mantid
PACKAGE=mantid
LABEL=nightly

# Handle flag inputs
while [ ! $# -eq 0 ]
do
    case "$1" in
        --channel)
            CHANNEL="$2"
            shift
            ;;
        --package)
            PACKAGE="$2"
            shift
            ;;
        --label)
            LABEL="$2"
            shift
            ;;
        *)
            echo "Argument not accepted: $1"
            exit 1
            ;;
  esac
  shift
done

EXPECTED_MAMBAFORGE_PATH=$WORKSPACE/mambaforge # Install into the WORKSPACE_DIR
if [[ $OSTYPE == "msys" ]]; then
    EXPECTED_CONDA_PATH=$EXPECTED_MAMBAFORGE_PATH/condabin/mamba.bat
else
    EXPECTED_CONDA_PATH=$EXPECTED_MAMBAFORGE_PATH/bin/mamba
fi
CONDA_ENV_NAME=mantid-anaconda-delete
RECIPES_DIR=$WORKSPACE/conda-recipes
SCRIPT_DIR=$WORKSPACE/buildconfig/Jenkins/Conda/

# Setup Mambaforge
$SCRIPT_DIR/download-and-install-mambaforge $EXPECTED_MAMBAFORGE_PATH $EXPECTED_CONDA_PATH true

# Remove conda env if it exists
$EXPECTED_CONDA_PATH env remove -n $CONDA_ENV_NAME

# Create env with anaconda-client installed
$EXPECTED_CONDA_PATH create -n $CONDA_ENV_NAME curl jq -y

# Activate Conda environment
. $WORKSPACE/mambaforge/etc/profile.d/conda.sh
conda activate $CONDA_ENV_NAME

FILE_URL="https://api.anaconda.org/package/$CHANNEL/$PACKAGE/files"
echo Get from url: $FILE_URL
ALL_PACKAGES=$(curl -s -X GET $FILE_URL)

LATEST_VERSION=$(echo $ALL_PACKAGES | jq -r '.[-1].version')
echo Latest Version: $LATEST_VERSION

PACKAGES_TO_DELETE=$(echo $ALL_PACKAGES | jq -r ".[] | select(.labels == [\"$LABEL\"] and .version != \"$LATEST_VERSION\") | .full_name")
echo Packages to delete: $PACKAGES_TO_DELETE

PACKAGES_NOT_TO_DELETE=$(echo $ALL_PACKAGES | jq -r ".[] | select(.labels == [\"$LABEL\"] and .version == \"$LATEST_VERSION\") | .full_name")
echo Packages not to delete: $PACKAGES_NOT_TO_DELETE

for package in $PACKAGES_TO_DELETE; do
    echo Deleting package: $package
    curl -s -X DELETE -H "Authorization: token $ANACONDA_TOKEN" "https://api.anaconda.org/dist/$package"
done