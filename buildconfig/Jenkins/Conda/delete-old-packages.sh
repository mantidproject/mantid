#!/bin/bash

# Delete all but the latest nightly build
#
# Expected args:
#   1. WORKSPACE: path to the workspace/source code that this should run inside
#   2. ANACONDA_TOKEN: The token required to interact with the mantid channel on anaconda.org
#
# Optional args:
#   --channel: The channel to remove from (defaults to mantid)
#   --label: The label to remove from the channel (defaults to nightly)
#
# All remaining arguments are package names to remove
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPT_DIR/mamba-utils

WORKSPACE=$1
ANACONDA_TOKEN=$2
shift 2

CHANNEL=mantid
LABEL=nightly
# Handle flag inputs
while [ ! $# -eq 0 ]
do
    case "$1" in
        --channel)
            CHANNEL="$2"
            shift
            ;;
        --label)
            LABEL="$2"
            shift
            ;;
        *)
            # Flags must come first so we have it
            # non-flag arguments so stop processing
            break
            ;;
  esac
  shift
done

###
# Delete a single named package from the given channel and label
function delete_package() {
  channel=$1
  package_name=$2
  label=$3

  file_url="https://api.anaconda.org/package/$channel/$package_name/files"
  echo Get from url: $file_url
  all_packages=$(curl -s -X GET $file_url)

  latest_version=$(echo $all_packages | jq -r '.[-1].version')
  echo Latest Version: $latest_version

  packages_to_delete=$(echo $all_packages | jq -r ".[] | select(.labels == [\"$label\"] and .version != \"$latest_version\") | .full_name")
  echo Packages to delete: $packages_to_delete

  packages_not_to_delete=$(echo $all_packages | jq -r ".[] | select(.labels == [\"$label\"] and .version == \"$latest_version\") | .full_name")
  echo Packages not to delete: $packages_not_to_delete

  for package_file in $packages_to_delete; do
      echo Deleting package: $package_file
      curl -s -X DELETE -H "Authorization: token $ANACONDA_TOKEN" "https://api.anaconda.org/dist/$package_file"
  done
}
###

# Mamba
setup_mamba $WORKSPACE/miniforge "deletion-anaconda"
mamba install --yes curl jq

for name in "$@"; do
  delete_package $CHANNEL $name $LABEL
done
