#!/bin/bash

# Construct a standalone windows MantidWorkbench NSIS package.
# The package is created from a pre-packaged conda version
# and removes any excess that is not necessary in a standalone
# packaged
set -e

# Constants
HERE="$(dirname "$0")"
BUILDDIR=_package_build
# Prefer mamba over conda for speed
CONDA_EXE=mamba
CONDA_PACKAGE=mantidworkbench

# Print usage and exit
function usage() {
  local exitcode=$1
  echo "Usage: $0 [options] package_name icon_file"
  echo
  echo "Create a DragNDrop packae out of a Conda package. The package is built in $BUILDDIR."
  echo "This directory will be created if it does not exist or purged if it already exists."
  echo "The final installer will be created in the current working directory."
  echo "Options:"
  echo "  -c Optional Conda channel overriding the default mantid"
  echo
  echo "Positional Arguments"
  echo "  package_name: The name of the package app directory, i.e. the final name will be '${package_name}.exe'"
  echo "  icon_file: An icon file, in icns format, for the final package"
  exit $exitcode
}

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
# 2) - the name of the package
# 3) - path to .icns image used as icon for package
package_name=$1
package_icon=$2

# Sanity check arguments. Especially ensure that paths are not empty as we are removing
# items and we don't want to accidentally clean out system paths
test -n "$package_name" || usage 1
test -n "$package_icon" || usage 1

package_dirname="$package_name"
package_contents="$BUILDDIR"/"$package_dirname"
echo "Building '$package_dirname' in '$BUILDDIR' from '$conda_channel' Conda channel"

# Build directory needs to be empty before we start
if [ -d "$BUILDDIR" ]; then
  echo "$BUILDDIR exists. Removing before commencing build."
  rm -fr "$BUILDDIR"
elif [ -e "$BUILDDIR" ]; then
  echo "$BUILDDIR exists but is not a directory. $BUILDDIR is expected to be created by this script as a place to build the package"
  echo "Either move/rename $BUILDDIR or use a different working directory"
  exit 1
fi

# Make directory because it needs to exist
echo "Making $BUILDDIR directory"
mkdir $BUILDDIR

# Create conda environment internally. --copy ensures no symlinks are used
package_conda_prefix="$package_contents"

echo "Creating Conda environment in '$package_conda_prefix' from '$CONDA_PACKAGE'"
"$CONDA_EXE" create --prefix "$package_conda_prefix" --copy --channel "$conda_channel" --yes \
  "$CONDA_PACKAGE" \
  jq  # used for processing the version string

# Determine version information
version=$("$CONDA_EXE" list --prefix "$package_conda_prefix" '^mantid$' --json | jq --raw-output '.[0].version')

# Remove jq
"$CONDA_EXE" remove --prefix "$package_conda_prefix" --yes jq

# Conda packages pretty much everything with each of its packages and much of this is
# unnecessary in this type of package - trim the fat.
echo "Purging '$package_conda_prefix' of unnecessary items"
# Heavily cut down everything in bin
mv "$package_conda_prefix"/bin "$package_conda_prefix"/bin_tmp
mkdir "$package_conda_prefix"/bin
cp "$package_conda_prefix"/bin_tmp/Mantid.properties "$package_conda_prefix"/bin/
cp "$package_conda_prefix"/bin_tmp/mantid-scripts.pth "$package_conda_prefix"/bin/
cp "$package_conda_prefix"/bin_tmp/MantidWorkbench "$package_conda_prefix"/bin/
cp "$package_conda_prefix"/bin_tmp/python "$package_conda_prefix"/bin/
cp "$package_conda_prefix"/bin_tmp/pip "$package_conda_prefix"/bin/
sed -i "" '1s|.*|#!/usr/bin/env python|' "$package_conda_prefix"/bin/pip
# Heavily cut down share
mv "$package_conda_prefix"/share "$package_conda_prefix"/share_tmp
# Removals
rm -rf "$package_conda_prefix"/bin_tmp \
  "$package_conda_prefix"/include \
  "$package_conda_prefix"/man \
  "$package_conda_prefix"/mkspecs \
  "$package_conda_prefix"/phrasebooks \
  "$package_conda_prefix"/qml \
  "$package_conda_prefix"/qsci \
  "$package_conda_prefix"/share_tmp \
  "$package_conda_prefix"/translations
find "$package_conda_prefix" -name 'qt.conf' -delete
find "$package_conda_prefix" -name '*.a' -delete
find "$package_conda_prefix" -name "*.pyc" -type f -delete
find "$package_conda_prefix" -path "*/__pycache__/*" -delete
find "$package_contents" -name '*.plist' -delete

# Add required resources
cp "$HERE"/BundleExecutable "$package_contents"/MacOS/"$package_name"
chmod +x "$package_contents"/MacOS/"$package_name"
cp "$HERE"/qt.conf "$package_conda_prefix"/bin/qt.conf
cp "$package_icon" "$package_conda_prefix"

# Create a plist from base version
package_plist="$package_contents"/Info.plist
cp "$HERE"/Info.plist.base "$package_plist"
add_string_to_plist "$package_plist" CFBundleIdentifier org.mantidproject."$package_name"
add_string_to_plist "$package_plist" CFBundleExecutable "$package_name"
add_string_to_plist "$package_plist" CFBundleName "$package_name"
add_string_to_plist "$package_plist" CFBundleIconFile "$(package_icon)"
add_string_to_plist "$package_plist" CFBundleVersion "$version"
add_string_to_plist "$package_plist" CFBundleLongVersionString "$version"

# Wrap up into dmg
version_name="$package_name"-"$version"
echo "Creating DragNDrop package '$version_name.dmg'"
hdiutil create -volname "$version_name" -srcfolder "$BUILDDIR" -ov -format UDZO "$version_name".dmg

echo
echo "Bundle '$package_name.dmg' successfully created."
