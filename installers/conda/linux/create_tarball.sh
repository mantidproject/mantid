#!/bin/bash

# Construct a standalone tarball for any modern linux system (2010+)
# The tarball is created from a pre-packaged conda version
# and removes any excess that is not necessary in a standalone
# version
set -e

# Constants
HERE="$(dirname "$0")"
BUILD_DIR=_bundle_build
CONDA_EXE=mamba
CONDA_PACKAGE=mantidworkbench
BUNDLE_PREFIX=mantidworkbench
ICON_DIR="$HERE/../../../images"

function trim_conda() {
  local bundle_conda_prefix=$1
  echo "Purging '$bundle_conda_prefix' of unnecessary items"
  # Heavily cut down everything in bin
  mv "$bundle_conda_prefix"/bin "$bundle_conda_prefix"/bin_tmp
  mkdir "$bundle_conda_prefix"/bin
  cp "$bundle_conda_prefix"/bin_tmp/Mantid.properties "$bundle_conda_prefix"/bin/
  cp "$bundle_conda_prefix"/bin_tmp/mantid-scripts.pth "$bundle_conda_prefix"/bin/
  cp "$bundle_conda_prefix"/bin_tmp/workbench "$bundle_conda_prefix"/bin/
  cp "$bundle_conda_prefix"/bin_tmp/mantidworkbench "$bundle_conda_prefix"/bin/
  cp "$bundle_conda_prefix"/bin_tmp/python* "$bundle_conda_prefix"/bin/
  cp "$bundle_conda_prefix"/bin_tmp/pip "$bundle_conda_prefix"/bin/
  sed -i -e '1s|.*|#!/usr/bin/env python|' "$bundle_conda_prefix"/bin/pip
  # Heavily cut down share
  mv "$bundle_conda_prefix"/share "$bundle_conda_prefix"/share_tmp
  mkdir "$bundle_conda_prefix"/share
  mv "$bundle_conda_prefix"/share_tmp/doc "$bundle_conda_prefix"/share/
  # Removals
  rm -rf "$bundle_conda_prefix"/bin_tmp \
    "$bundle_conda_prefix"/include \
    "$bundle_conda_prefix"/man \
    "$bundle_conda_prefix"/mkspecs \
    "$bundle_conda_prefix"/phrasebooks \
    "$bundle_conda_prefix"/qml \
    "$bundle_conda_prefix"/qsci \
    "$bundle_conda_prefix"/share_tmp \
    "$bundle_conda_prefix"/translations
  find "$bundle_conda_prefix" -name 'qt.conf' -delete
  find "$bundle_conda_prefix" -name '*.a' -delete
  find "$bundle_conda_prefix" -name "*.pyc" -type f -delete
  find "$bundle_conda_prefix" -path "*/__pycache__/*" -delete
  find "$bundle_contents" -name '*.plist' -delete
}

# Add required resources into bundle
#   $1 - Location of bundle root
#   $2 - Path to icon to copy
function add_resources() {
  local bundle_root=$1
  local bundle_icon=$2
  echo "Adding additional required resources"
  cp "$HERE"/qt.conf "$bundle_root"/bin/qt.conf
  mkdir -p "$bundle_root"/share/pixmaps
  cp "$bundle_icon" "$bundle_root"/share/pixmaps
}

# Fixup bundle so it is self-contained
#  $1 - Root directory of the bundle
function fixup_bundle() {
  local bundle_root_absolute=$(readlink -f "$1")
  echo "Fixing up bundle to be self contained"
  # Startup script contains absolute paths to installed conda prefix,
  # replace with relative path to INSTALLDIR defined in the script itself
  sed -i -e "s@$bundle_root_absolute/@\$INSTALLDIR/@" $bundle_root_absolute/bin/mantidworkbench
}

# Create a tarball out of the installed conda environment
#  $1 - Name of the final file (without the suffix)
#  $2 - Bundle name
function create_tarball() {
  local version_name=$1
  local bundle_name=$2
  echo "Creating tarball '$version_name.xz'"
  tar --directory "$BUILD_DIR" --create --file "$version_name.xz" --xz "$bundle_name"
}

# Print usage and exit
function usage() {
  local exitcode=$1
  echo "Usage: $0 [options]"
  echo
  echo "Create a tarball bundle out of a Conda package. The package is built in $BUILD_DIR."
  echo "The final name will be '${BUNDLE_PREFIX}{suffix}.xz'"
  echo "This directory will be created if it does not exist or purged if it already exists."
  echo "The final .dmg will be created in the current working directory."
  echo "Options:"
  echo "  -c Optional Conda channel overriding the default mantid"
  echo "  -s Optional Add a suffix to the output mantid file, has to be Unstable, or Nightly or not used"
  echo
  exit $exitcode
}

## Script begin
# Optional arguments
conda_channel=mantid
suffix=
while [ ! $# -eq 0 ]
do
    case "$1" in
        -c)
            conda_channel="$2"
            shift
            ;;
        -s)
            suffix="$2"
            shift
            ;;
        -h)
            usage 0
            ;;
        *)
            if [ ! -z "$2" ]
            then
              usage 1
            fi
            ;;
  esac
  shift
done

# If suffix is not empty and does not contain Unstable or Nightly then fail.
if [ ! -z "$suffix" ]; then
  if [ "$suffix" != "Unstable" ] && [ "$suffix" != "Nightly" ]; then
    echo "Suffix must either not be passed, or be Unstable or Nightly, for release do not pass this argument."
    exit 1
  fi
fi

suffix_lower=$(echo $suffix | tr '[:upper:]' '[:lower:]')
bundle_name="$BUNDLE_PREFIX$suffix_lower"
bundle_icon="${ICON_DIR}/mantid_workbench${suffix_lower}.png"
bundle_dirname="$bundle_name"
bundle_contents="$BUILD_DIR"/"$bundle_dirname"
echo "Building '$bundle_dirname' in '$BUILD_DIR' from '$conda_channel' Conda channel"
echo "Using bundle icon ${bundle_icon}"

# Build directory needs to be empty before we start
if [ -d "$BUILD_DIR" ]; then
  echo "$BUILD_DIR exists. Removing before commencing build."
  rm -fr "$BUILD_DIR"
elif [ -e "$BUILD_DIR" ]; then
  echo "$BUILD_DIR exists but is not a directory. $BUILD_DIR is expected to be created by this script as a place to build the bundle"
  echo "Either move/rename $BUILD_DIR or use a different working directory"
  exit 1
fi

# Create base directory.
mkdir -p "$bundle_contents"

# Create conda environment internally. --copy ensures no symlinks are used
bundle_conda_prefix="$bundle_contents"

echo "Creating Conda environment in '$bundle_conda_prefix' from '$CONDA_PACKAGE'"
"$CONDA_EXE" create --quiet --prefix "$bundle_conda_prefix" --copy \
  --channel "$conda_channel" --channel conda-forge --yes \
  "$CONDA_PACKAGE" \
  jq  # used for processing the version string
echo

# Determine version information
version=$("$CONDA_EXE" list --prefix "$bundle_conda_prefix" '^mantid$' --json | \
  "$bundle_conda_prefix"/bin/jq --raw-output --args '.[0].version' "$mantid_pkg_info")
echo "Bundling mantid version $version"
echo

# Remove jq
"$CONDA_EXE" remove --quiet --prefix "$bundle_conda_prefix" --yes jq

# Fixup bundle
fixup_bundle "$bundle_contents" "$bundle_icon"

# Conda bundles pretty much everything with each of its packages and much of this is
# unnecessary in this type of bundle - trim the fat.
trim_conda "$bundle_conda_prefix"

# Add any additional resources
add_resources "$bundle_contents" "$bundle_icon"

# Create tarball and compress
version_name="$bundle_name"-"$version"
create_tarball "$version_name" "$bundle_name"
