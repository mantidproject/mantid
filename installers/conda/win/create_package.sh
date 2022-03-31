#!/bin/bash -e

# REQUIRES NSIS to be installed and on the path.
# Construct a standalone windows MantidWorkbench NSIS package.
# The package is created from a pre-packaged conda version
# and removes any excess that is not necessary in a standalone
# packaged

# Print usage and exit
function usage() {
  local exitcode=$1
  echo "Usage: $0 [options] package_name icon_file"
  echo
  echo "Create a standalone installable package out of a mantidworkbench Conda package. The"
  echo " package is built in $BUILDDIR. This directory will be created if it does not exist"
  echo " or purged if it already exists. The final installer will be created in the current"
  echo " working directory. Requires mamba and NSIS to be installed in the running "
  echo "environment, and on the path."
  echo "Options:"
  echo "  -c Optional Conda channel overriding the default mantid"
  echo
  echo "Positional Arguments"
  echo "  package_name: The name of the package exe, i.e. the final name will be '${package_name}.exe'"
  exit $exitcode
}

# Optional arguments
CONDA_CHANNEL=mantid
while getopts ":c:h" o; do
  case "$o" in
  c) CONDA_CHANNEL="$OPTARG";;
  h) usage 0;;
  *) usage 1;;
esac
done
shift $((OPTIND-1))

# Define variables
THIS_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONDA_ENV=_conda_env
CONDA_ENV_PATH=$THIS_SCRIPT_DIR/$CONDA_ENV
COPY_DIR=$THIS_SCRIPT_DIR/_package_build
CONDA_EXE=mamba
PACKAGE_NAME=$1

# Sanity check arguments. Especially ensure that paths are not empty as we are removing
# items and we don't want to accidentally clean out system paths
test -n "$PACKAGE_NAME" || usage 1

echo "Cleaning up left over old directories"
rm -rf $COPY_DIR
rm -rf $CONDA_ENV_PATH

mkdir $COPY_DIR

echo "Creating conda env from mantidworkbench and jq"
"$CONDA_EXE" create --prefix $CONDA_ENV_PATH mantidworkbench m2w64-jq --copy -c $CONDA_CHANNEL -c conda-forge -y
echo "Conda env created"

# Determine version information
VERSION=$("$CONDA_EXE" list --prefix "$CONDA_ENV_PATH" '^mantid$' --json | $CONDA_ENV_PATH/Library/mingw-w64/bin/jq.exe --raw-output '.[0].version')
echo "Version number: $version"

# Remove jq
echo "Removing jq from conda env"
"$CONDA_EXE" remove --prefix $CONDA_ENV_PATH --yes m2w64-jq
echo "jq removed from conda env"

# Pip install quasielasticbayes so it can be packaged alongside workbench on windows
$CONDA_ENV_PATH/python.exe -m pip install quasielasticbayes

echo "Copying root packages of env files (Python, DLLs, Lib, Scripts, ucrt, and msvc files) to package/bin"
cp $CONDA_ENV_PATH/DLLs $COPY_DIR/bin/ -r
cp $CONDA_ENV_PATH/Lib $COPY_DIR/bin/ -r
cp $CONDA_ENV_PATH/Scripts $COPY_DIR/bin/ -r
cp $CONDA_ENV_PATH/tcl $COPY_DIR/bin/ -r
cp $CONDA_ENV_PATH/python*.* $COPY_DIR/bin/
cp $CONDA_ENV_PATH/msvc*.* $COPY_DIR/bin/
cp $CONDA_ENV_PATH/ucrt*.* $COPY_DIR/bin/

echo "Copying mantid python files into bin"
cp $CONDA_ENV_PATH/Lib/site-packages/mantid* $COPY_DIR/bin/ -r
cp $CONDA_ENV_PATH/Lib/site-packages/ $COPY_DIR/bin/ -r
cp $CONDA_ENV_PATH/Lib/site-packages/workbench $COPY_DIR/bin/workbench -r

echo "Copy all DLLs from env/Library/bin to package/bin"
cp $CONDA_ENV_PATH/Library/bin/*.dll $COPY_DIR/bin/

echo "Copy Mantid specific files from env/Library/bin to package/bin"
cp $CONDA_ENV_PATH/Library/bin/Mantid.properties $COPY_DIR/bin/
cp $CONDA_ENV_PATH/Library/bin/Mantid.user.properties $COPY_DIR/bin/
cp $CONDA_ENV_PATH/Library/bin/MantidNexusParallelLoader.exe $COPY_DIR/bin/
cp $CONDA_ENV_PATH/Library/bin/mantid-scripts.pth $COPY_DIR/bin/
cp $CONDA_ENV_PATH/Library/bin/MantidWorkbench.exe $COPY_DIR/bin/

echo "Copy env/includes to the package/includes"
cp $CONDA_ENV_PATH/Library/include/eigen3 $COPY_DIR/include/ -r

echo "Copy Instrument details to the package"
cp $CONDA_ENV_PATH/Library/instrument $COPY_DIR/ -r

echo "Constructing package/lib/qt5"
mkdir $COPY_DIR/lib
mkdir $COPY_DIR/lib/qt5
mkdir $COPY_DIR/lib/qt5/bin
cp $CONDA_ENV_PATH/Library/bin/QtWebEngineProcess.exe $COPY_DIR/lib/qt5/bin
cp $CONDA_ENV_PATH/Library/bin/qt.conf $COPY_DIR/lib/qt5/bin
cp $CONDA_ENV_PATH/Library/resources $COPY_DIR/lib/qt5/ -r

echo "Copy plugins to the package"
mkdir $COPY_DIR/plugins
mkdir $COPY_DIR/plugins/qt5
cp $CONDA_ENV_PATH/Library/plugins/platforms $COPY_DIR/plugins/qt5/ -r
cp $CONDA_ENV_PATH/Library/plugins/imageformats $COPY_DIR/plugins/qt5/ -r
cp $CONDA_ENV_PATH/Library/plugins/printsupport $COPY_DIR/plugins/qt5/ -r
cp $CONDA_ENV_PATH/Library/plugins/sqldrivers $COPY_DIR/plugins/qt5/ -r
cp $CONDA_ENV_PATH/Library/plugins/styles $COPY_DIR/plugins/qt5/ -r
cp $CONDA_ENV_PATH/Library/plugins/*.dll $COPY_DIR/plugins/
cp $CONDA_ENV_PATH/Library/plugins/python $COPY_DIR/plugins/ -r

echo "Copy scripts into the package"
cp $CONDA_ENV_PATH/Library/scripts $COPY_DIR/ -r

echo "Copy share files (includes mantid docs) to the package"
cp $CONDA_ENV_PATH/Library/share/doc $COPY_DIR/share/ -r
cp $CONDA_ENV_PATH/Library/share/eigen3 $COPY_DIR/share/ -r

echo "Copy executable file and executable script into package"
cp $THIS_SCRIPT_DIR/MantidWorkbench.exe $COPY_DIR/bin/ -f

# Cleanup pdb files and remove them from bin
echo "Performing some cleanup.... deleting files"
rm -rf $COPY_DIR/bin/*.pdb
find $COPY_DIR -name *.pyc -delete
# Delete extra DLLs
rm -rf $COPY_DIR/bin/api-ms-win*.dll
rm -rf $COPY_DIR/bin/libclang.dll

echo "Cleanup directory containing build that is no longer needed"
rm -rf $CONDA_ENV_PATH

# Now package using NSIS
echo "Edit the NSIS script"
NSIS_SCRIPT=$THIS_SCRIPT_DIR/project.nsi

echo "Packaging package via NSIS"
# Make windows-like paths because NSIS is weird
SCRIPT_DRIVE_LETTER="$(echo ${COPY_DIR:1:1} | tr [:lower:] [:upper:])"
COPY_DIR=${COPY_DIR////\\}
COPY_DIR="$SCRIPT_DRIVE_LETTER:${COPY_DIR:2}"

NSIS_SCRIPT=${NSIS_SCRIPT////\\}
NSIS_SCRIPT="$SCRIPT_DRIVE_LETTER:${NSIS_SCRIPT:2}"

ICON_PATH=$THIS_SCRIPT_DIR/../../../images/mantidplot.ico
ICON_PATH=${ICON_PATH////\\}
ICON_PATH="$SCRIPT_DRIVE_LETTER:${ICON_PATH:2}"

LICENSE_PATH=$THIS_SCRIPT_DIR/../../../LICENSE.txt
LICENSE_PATH=${LICENSE_PATH////\\}
LICENSE_PATH="$SCRIPT_DRIVE_LETTER:${LICENSE_PATH:2}"

echo makensis /V4 /DVERSION=$VERSION /DPACKAGE_DIR=\"$COPY_DIR\" /DOUTFILE_NAME=$PACKAGE_NAME /DICON_PATH=$ICON_PATH /DMUI_PAGE_LICENSE_PATH=$LICENSE_PATH \"$NSIS_SCRIPT\"
cmd.exe /C "START /wait "" makensis /V4 /DVERSION=$VERSION /DPACKAGE_DIR=\"$COPY_DIR\" /DOUTFILE_NAME=$PACKAGE_NAME /DICON_PATH=$ICON_PATH /DMUI_PAGE_LICENSE_PATH=$LICENSE_PATH \"$NSIS_SCRIPT\""
echo "Package packaged, find it here: $THIS_SCRIPT_DIR/$PACKAGE_NAME"

echo "Cleaning up left over files"
rm -rf $CONDA_ENV_PATH
rm -rf $COPY_DIR

echo "Done"