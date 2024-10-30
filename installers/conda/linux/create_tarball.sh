#!/bin/bash
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# Construct a standalone tarball for any modern linux system (2010+)
# The tarball is created from a pre-packaged conda version
# and removes any excess that is not necessary in a standalone
# version
set -e

# Constants
HERE="$(dirname "$0")"
BUILD_DIR=_bundle_build
CONDA_EXE=mamba
BUNDLE_PREFIX=mantidworkbench
BUNDLE_EXTENSION=.tar.xz
ICON_DIR="$HERE/../../../images"

# Common routines
source $HERE/../common/common.sh

# Add required resources into bundle
#   $1 - Location of bundle root
#   $2 - Path to icon to copy
function add_resources() {
  local bundle_root=$1
  local bundle_icon=$2
  echo "Adding additional required resources"
  mkdir -p "$bundle_root"/share/pixmaps
  cp "$bundle_icon" "$bundle_root"/share/pixmaps
}

# Fixup bundle so it is self-contained
#  $1 - Root directory of the bundle
function fixup_bundle() {
  local bundle_conda_prefix=$1
  local bundle_prefix_absolute=$(readlink -f "$1")
  echo "Fixing up bundle so it is self contained"
  # Fix absolute paths in Qt and our own startup script
  fixup_qt "$bundle_conda_prefix" "$HERE"/../common/qt.conf
  sed -i -e "s@$bundle_prefix_absolute/@\$INSTALLDIR/@" $bundle_prefix_absolute/bin/mantidworkbench
}

# Create a tarball out of the installed conda environment
#  $1 - Name of the final file (without the suffix)
#  $2 - Bundle name
function create_tarball() {
  local version_filename=$1$BUNDLE_EXTENSION
  local bundle_name=$2
  echo "Creating tarball '$version_filename'"
  tar --directory "$BUILD_DIR" --create --file "$version_filename" --xz "$bundle_name"
}

# Print usage and exit
function usage() {
  local exitcode=$1
  echo "Usage: $0 [options]"
  echo
  echo "Create a tarball bundle out of a Conda package. The package is built in $BUILD_DIR."
  echo "The final name will be '${BUNDLE_PREFIX}{suffix}${BUNDLE_EXTENSION}'"
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

echo "Creating Conda environment in '$bundle_conda_prefix'"
"$CONDA_EXE" create --quiet --prefix "$bundle_conda_prefix" --copy \
  --channel "$conda_channel" --channel conda-forge --channel mantid --yes \
  mantidworkbench \
  jq  # used for processing the version string
echo

# Determine version information
version=$("$CONDA_EXE" list --prefix "$bundle_conda_prefix" '^mantid$' --json | \
  "$bundle_conda_prefix"/bin/jq --raw-output --args '.[0].version' "$mantid_pkg_info")
echo "Bundling mantid version $version"
echo

# Remove jq
"$CONDA_EXE" remove --quiet --prefix "$bundle_conda_prefix" --yes jq

# Pip install quickBayes until there's a conda package
$bundle_conda_prefix/bin/python -m pip install quickBayes==1.0.0b15

# Trim and fixup bundle
trim_conda "$bundle_conda_prefix"
fixup_bundle "$bundle_conda_prefix" "$bundle_icon"
add_resources "$bundle_conda_prefix" "$bundle_icon"

# Create tarball and compress
version_name="$bundle_name"-"$version"
create_tarball "$version_name" "$bundle_name"
