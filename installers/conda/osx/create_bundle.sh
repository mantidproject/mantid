#!/bin/bash

# Construct a standalone macOS MantidWorkbench .dmg bundle.
# The bundle is created from a pre-packaged conda version
# and removes any excess that is not necessary in a standalone
# bundle
set -e

# Constants
HERE="$(dirname "$0")"
BUILD_DIR=_bundle_build
TEMP_IMAGE=/tmp/temp.dmg
TEMP_OSA_SCPT=/tmp/dmg_setup.scpt
DETACH_MAX_TRIES=5
DETACH_WAIT_SECS=60
CONDA_EXE=mamba
CONDA_PACKAGE=mantidworkbench

# Cleanup on script exit
# Remove temporary work files and ensure volumes are detached
#   $1 - The name of the mounted volume that, if it still exists, should be detached
function cleanup() {
  ensure_volume_detached "$1"
  rm -f "$TEMP_OSA_SCPT"
  rm -f "$TEMP_IMAGE"
}

# Attempt to detach the named volume. This will wait for a maximum of 5 minutes
#   $1 - The name of the mounted volume that, if it still exists, should be detached
function ensure_volume_detached() {
  local volume_name="$1"
  if [ ! -d /Volumes/"$volume_name" ]; then
    return
  fi

  # It can quite often happen that the resource is busy on first try.
  # We keep trying for several times
  local counter=0
  while [ $counter -lt $DETACH_MAX_TRIES ]; do
    if hdiutil detach /Volumes/"$volume_name"; then
      break
    fi
    (( counter+=1 ))
    sleep $DETACH_WAIT_SECS
  done
}

# Use otool/install_name_tool to change a dependent path
# to a reexported library. The original is assumed to contain
# an absolute path
#   $1 - Full Path to the .so/.dylib to fix
function fixup_reexport_dependent_path() {
  local libpath=$1
  local -r dependent_path=$(otool -l "${libpath}" | grep LC_REEXPORT_DYLIB -A 2 | grep name | awk '{print $2}')
  local -r dependent_fname=$(basename "${dependent_path}")

  echo "Fixing up LC_REEXPORT_DYLIB path in '$libpath'"
  install_name_tool -change "${dependent_path}" @rpath/"${dependent_fname}" "${libpath}"
}

function trim_conda() {
  local bundle_conda_prefix=$1
  echo "Purging '$bundle_conda_prefix' of unnecessary items"
  # Heavily cut down everything in bin
  mv "$bundle_conda_prefix"/bin "$bundle_conda_prefix"/bin_tmp
  mkdir "$bundle_conda_prefix"/bin
  cp "$bundle_conda_prefix"/bin_tmp/Mantid.properties "$bundle_conda_prefix"/bin/
  cp "$bundle_conda_prefix"/bin_tmp/mantid-scripts.pth "$bundle_conda_prefix"/bin/
  cp "$bundle_conda_prefix"/bin_tmp/MantidWorkbench "$bundle_conda_prefix"/bin/
  cp "$bundle_conda_prefix"/bin_tmp/python "$bundle_conda_prefix"/bin/
  cp "$bundle_conda_prefix"/bin_tmp/pip "$bundle_conda_prefix"/bin/
  sed -i "" '1s|.*|#!/usr/bin/env python|' "$bundle_conda_prefix"/bin/pip
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
#   $1 - Location of bundle contents directory
#   $2 - Name of the bundle (part before .app)
#   $3 - Path to icon to copy
function add_resources() {
  local bundle_contents=$1
  local bundle_name=$2
  local bundle_icon=$3
  cp "$HERE"/BundleExecutable "$bundle_contents"/MacOS/"$bundle_name"
  chmod +x "$bundle_contents"/MacOS/"$bundle_name"
  cp "$HERE"/qt.conf "$bundle_contents"/Resources/bin/qt.conf
  cp "$bundle_icon" "$bundle_contents"/Resources
}

# Add required resources into bundle
#   $1 - Location of bundle contents directory
#   $2 - Name of the bundle (part before .app)
#   $3 - Path to icon (only the basename is used)
#   $4 - The version of the bundle
function create_plist() {
  local bundle_contents=$1
  local bundle_name=$2
  local bundle_icon=$3
  local version=$4
  bundle_plist="$bundle_contents"/Info.plist
  cp "$HERE"/Info.plist.base "$bundle_plist"
  add_string_to_plist "$bundle_plist" CFBundleIdentifier org.mantidproject."$bundle_name"
  add_string_to_plist "$bundle_plist" CFBundleExecutable "$bundle_name"
  add_string_to_plist "$bundle_plist" CFBundleName "$bundle_name"
  add_string_to_plist "$bundle_plist" CFBundleIconFile "$(basename $bundle_icon)"
  add_string_to_plist "$bundle_plist" CFBundleVersion "$version"
  add_string_to_plist "$bundle_plist" CFBundleLongVersionString "$version"
}

# Add a key string-value pair to a plist
#  $1 - Full path to the plist
#  $2 - Key name
#  $3 - Key string value
function add_string_to_plist() {
  local filepath=$1
  local key=$2
  local string_value=$3
  /usr/libexec/PlistBuddy -c "Add :$key string $string_value" "$filepath"
}

function create_disk_image() {
  local version_name=$1
  echo "Creating disk image '$version_name.dmg'"
  # Add additional DragNDrop facilities
  #   Add link to Applications
  #   Pad out the image so that we can make changes went mounted
  #   Store a background image
  ln -s /Applications "$BUILD_DIR"/Applications
  mkdir "$BUILD_DIR/.background"
  cp "$HERE/dmg_background.png" "$BUILD_DIR/.background/background.png"
  padding_filename=".dummy-padding-file"
  mkfile 1m "$BUILD_DIR/$padding_filename"

  # Set Finder background
  hdiutil create -ov -srcfolder "$BUILD_DIR" -volname "$version_name" -fs HFS+ -format UDRW "$TEMP_IMAGE"
  volume_name=$(hdiutil attach "$TEMP_IMAGE" | perl -n -e '/\/Volumes\/(.+)/ && print $1')
  trap "cleanup $volume_name" RETURN
  volume_path=/Volumes/"$volume_name"
  rm "$volume_path"/"$padding_filename"
  sed -e "s/@BUNDLE_NAME@/$bundle_name/" "$HERE/dmg_setup.scpt.in" > "$TEMP_OSA_SCPT"
  osascript "$TEMP_OSA_SCPT" "$version_name"
  ensure_volume_detached "$volume_name"

  # Create final compressed dmg
  test -f "$version_name".dmg && rm -f "$version_name".dmg
  hdiutil convert "$TEMP_IMAGE" -format UDZO -imagekey zlib-level=9 -o "$version_name".dmg
}

# Print usage and exit
function usage() {
  local exitcode=$1
  echo "Usage: $0 [options] bundle_name icon_file"
  echo
  echo "Create a DragNDrop bundle out of a Conda package. The package is built in $BUILD_DIR."
  echo "This directory will be created if it does not exist or purged if it already exists."
  echo "The final .dmg will be created in the current working directory."
  echo "Options:"
  echo "  -c Optional Conda channel overriding the default mantid"
  echo
  echo "Positional Arguments"
  echo "  bundle_name: The name of the bundle app directory, i.e. the final name will be '${bundle_name}.app'"
  echo "  icon_file: An icon file, in icns format, for the final bundle"
  exit $exitcode
}

## Script begin
# Optional arguments
conda_channel=mantid
while getopts ":c:h" o; do
  case "$o" in
  c) conda_channel="$OPTARG";;
  h) usage 0;;
  *) usage 1;;
esac
done
shift $((OPTIND-1))

# Positional arguments
# 2) - the name of the bundle
# 3) - path to .icns image used as icon for bundle
bundle_name=$1
bundle_icon=$2

# Sanity check arguments. Especially ensure that paths are not empty as we are removing
# items and we don't want to accidentally clean out system paths
test -n "$bundle_name" || usage 1
test -n "$bundle_icon" || usage 1

bundle_dirname="$bundle_name".app
bundle_contents="$BUILD_DIR"/"$bundle_dirname"/Contents
echo "Building '$bundle_dirname' in '$BUILD_DIR' from '$conda_channel' Conda channel"

# Build directory needs to be empty before we start
if [ -d "$BUILD_DIR" ]; then
  echo "$BUILD_DIR exists. Removing before commencing build."
  rm -fr "$BUILD_DIR"
elif [ -e "$BUILD_DIR" ]; then
  echo "$BUILD_DIR exists but is not a directory. $BUILD_DIR is expected to be created by this script as a place to build the bundle"
  echo "Either move/rename $BUILD_DIR or use a different working directory"
  exit 1
fi

# Create basic directory structure.
mkdir -p "$bundle_contents"/{Resources,MacOS}

# Create conda environment internally. --copy ensures no symlinks are used
bundle_conda_prefix="$bundle_contents"/Resources

echo "Creating Conda environment in '$bundle_conda_prefix' from '$CONDA_PACKAGE'"
"$CONDA_EXE" create --quiet --prefix "$bundle_conda_prefix" --copy --channel "$conda_channel" --yes \
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

# Certain packages use LC_REEXPORT_DYLIB rather than LC_LOAD_DYLIB for their dependents
# See http://blog.darlinghq.org/2018/07/mach-o-linking-and-loading-tricks.html for details.
# Conda install rewrites these LC_REEXPORT_DYLIB links with absolute links and breaks relocation
# This can easily be remedied by switching to use the @rpath identifier
# We avoid scanning the whole bundle as this potentially very expensive and instead fixup the libraries
# we know have issues
fixup_reexport_dependent_path "$(find "$bundle_conda_prefix" -name 'libncurses.*' -type f)"

# Conda bundles pretty much everything with each of its packages and much of this is
# unnecessary in this type of bundle - trim the fat.
trim_conda "$bundle_conda_prefix"

# Add required resources
add_resources "$bundle_contents" "$bundle_name" "$bundle_icon"

# Create a plist from base version
create_plist "$bundle_contents" "$bundle_name" "$bundle_icon" "$version"

# Create DMG, including custom background and /Applications link
# These steps were extracted from how cpack achieves this:
# https://github.com/Kitware/CMake/blob/859241d2bbaae83f06c44bc21ab0dbd38725a3fc/Source/CPack/cmCPackDragNDropGenerator.cxx
# Steps:
#   - Create dummy 1MB file for space on the filesystem to store changes
#   - Create temporary dmgtrap
#   - Mount temporary dmg
#   - Remove dummy padding
#   - Execute apple script to set background and create .DS_Store in mounted directory
#   - Detach temporary image
#   - Create final, compressed image
version_name="$bundle_name"-"$version"
create_disk_image "$version_name"
