#!/bin/bash -e

# Constructs a standalone windows MantidWorkbench installer using NSIS.
# The package is created from a pre-packaged version of Mantid from conda
# and removes some excess that is not necessary in a standalone
# package

# Print usage and exit
function usage() {
  local exitcode=$1
  echo "Usage: $0 [options] package_name"
  echo
  echo "Create a standalone installable package out of a mantidworkbench Conda package."
  echo "Requires mamba to be installed in the running environment, and on the path."
  echo "Options:"
  echo "  -c Optional Conda channel overriding the default mantid"
  echo "  -s Optional Add a suffix to the output mantid file, has to be Unstable, or Nightly or not used"
  echo
  echo "Positional Arguments"
  echo "  package_name: The name of the package exe, i.e. the final name will be '${package_name}.exe'"
  exit $exitcode
}

# Optional arguments
CONDA_CHANNEL=mantid
SUFFIX=""
while [ ! $# -eq 0 ]
do
    case "$1" in
        -c)
            CONDA_CHANNEL="$2"
            shift
            ;;
        -s)
            SUFFIX="$2"
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
if [ ! -z "$SUFFIX" ]; then
  if [ "$SUFFIX" != "Unstable" ] && [ "$SUFFIX" != "Nightly" ]; then
    echo "Suffix must either not be passed, or be Unstable or Nightly, for release do not pass this argument."
    exit 1
  fi
fi

# Define variables
THIS_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONDA_ENV=_conda_env
CONDA_ENV_PATH=$THIS_SCRIPT_DIR/$CONDA_ENV
COPY_DIR=$THIS_SCRIPT_DIR/_package_build
CONDA_EXE=mamba
PACKAGE_PREFIX=MantidWorkbench
PACKAGE_NAME="$PACKAGE_PREFIX${SUFFIX}"
LOWER_CASE_SUFFIX="$(echo ${SUFFIX} | tr [:upper:] [:lower:])"

echo "Cleaning up left over old directories"
rm -rf $COPY_DIR
rm -rf $CONDA_ENV_PATH

mkdir $COPY_DIR

echo "Creating conda env from mantidworkbench and jq"
"$CONDA_EXE" create --prefix $CONDA_ENV_PATH \
  --always-copy --channel $CONDA_CHANNEL --channel conda-forge -y \
  mantidworkbench \
  m2w64-jq
echo "Conda env created"

# Determine version information
VERSION=$("$CONDA_EXE" list --prefix "$CONDA_ENV_PATH" '^mantid$' --json | $CONDA_ENV_PATH/Library/mingw-w64/bin/jq.exe --raw-output '.[0].version')
echo "Version number: $VERSION"
echo "Removing jq from conda env"
"$CONDA_EXE" remove --prefix $CONDA_ENV_PATH --yes m2w64-jq
echo "jq removed from conda env"

# Pip install quickBayes until there's a conda package
$CONDA_ENV_PATH/python.exe -m pip install quickBayes==1.0.0b15


echo "Copying root packages of env files (Python, DLLs, Lib, Scripts, ucrt, and msvc files) to package/bin"
mkdir $COPY_DIR/bin
mv $CONDA_ENV_PATH/DLLs $COPY_DIR/bin/
mv $CONDA_ENV_PATH/Lib $COPY_DIR/bin/
mv $CONDA_ENV_PATH/Scripts $COPY_DIR/bin/
mv $CONDA_ENV_PATH/python*.* $COPY_DIR/bin/
mv $CONDA_ENV_PATH/msvc*.* $COPY_DIR/bin/
mv $CONDA_ENV_PATH/ucrt*.* $COPY_DIR/bin/

echo "Copy all DLLs from env/Library/bin to package/bin"
mv $CONDA_ENV_PATH/Library/bin/*.dll $COPY_DIR/bin/

echo "Copy Mantid specific files from env/Library/bin to package/bin"
mv $CONDA_ENV_PATH/Library/bin/Mantid.properties $COPY_DIR/bin/
mv $CONDA_ENV_PATH/Library/bin/MantidNexusParallelLoader.exe $COPY_DIR/bin/
mv $CONDA_ENV_PATH/Library/bin/mantid-scripts.pth $COPY_DIR/bin/

echo "Copy Mantid icon files from source to package/bin"
cp $THIS_SCRIPT_DIR/../../../images/mantid_workbench$LOWER_CASE_SUFFIX.ico $COPY_DIR/bin/mantid_workbench.ico

echo "Copy Instrument details to the package"
mv $CONDA_ENV_PATH/Library/instrument $COPY_DIR/

echo "Constructing package/lib/qt5"
mkdir -p $COPY_DIR/lib/qt5/bin
mv $CONDA_ENV_PATH/Library/bin/QtWebEngineProcess.exe $COPY_DIR/lib/qt5/bin
cp $THIS_SCRIPT_DIR/../common/qt.conf $COPY_DIR/lib/qt5/bin
mv $CONDA_ENV_PATH/Library/resources $COPY_DIR/lib/qt5/
mkdir -p $COPY_DIR/lib/qt5/translations/qtwebengine_locales
mv $CONDA_ENV_PATH/Library/translations/qtwebengine_locales/en*.pak $COPY_DIR/lib/qt5/translations/qtwebengine_locales/

echo "Copy plugins to the package"
mkdir $COPY_DIR/plugins
mkdir $COPY_DIR/plugins/qt5
mv $CONDA_ENV_PATH/Library/plugins/platforms $COPY_DIR/plugins/qt5/
mv $CONDA_ENV_PATH/Library/plugins/imageformats $COPY_DIR/plugins/qt5/
mv $CONDA_ENV_PATH/Library/plugins/printsupport $COPY_DIR/plugins/qt5/
mv $CONDA_ENV_PATH/Library/plugins/sqldrivers $COPY_DIR/plugins/qt5/
mv $CONDA_ENV_PATH/Library/plugins/styles $COPY_DIR/plugins/qt5/
mv $CONDA_ENV_PATH/Library/plugins/qt5/*.dll $COPY_DIR/plugins/qt5/
mv $CONDA_ENV_PATH/Library/plugins/*.dll $COPY_DIR/plugins/
mv $CONDA_ENV_PATH/Library/plugins/python $COPY_DIR/plugins/

echo "Copy scripts into the package"
mv $CONDA_ENV_PATH/Library/scripts $COPY_DIR/

echo "Copy share files (includes mantid docs) to the package"
mkdir $COPY_DIR/share
mv $CONDA_ENV_PATH/share/doc/* $COPY_DIR/share/

echo "Copy executable launcher"
# MantidWorkbench-script.pyw is created by project.nsi on creation of the package
cp $THIS_SCRIPT_DIR/MantidWorkbench.exe $COPY_DIR/bin/

echo "Copy site customization module"
# Adds a sitecustomize module to ensure the bin directory
# is added to the DLL load PATH
cp $THIS_SCRIPT_DIR/sitecustomize.py $COPY_DIR/bin/Lib/site-packages/

# Cleanup pdb files and remove them from bin
echo "Performing some cleanup.... deleting files"
rm -rf $COPY_DIR/bin/*.pdb
find $COPY_DIR -name *.pyc -delete
# Delete extra DLLs
rm -rf $COPY_DIR/bin/api-ms-win*.dll
rm -rf $COPY_DIR/bin/libclang.dll

# Now package using NSIS
echo "Packaging package via NSIS"

# Create a conda environment with nsis installed
NSIS_CONDA_ENV=_nsis_conda_env
NSIS_CONDA_ENV_PATH=$THIS_SCRIPT_DIR/$NSIS_CONDA_ENV
# First remove existing environment if it exists
rm -rf $NSIS_CONDA_ENV_PATH
# Remove temporary nsis helper files
rm -f $THIS_SCRIPT_DIR/uninstall_files.nsh
rm -f $THIS_SCRIPT_DIR/uninstall_dirs.nsh

echo "Creating nsis conda env"
"$CONDA_EXE" create --prefix $NSIS_CONDA_ENV_PATH nsis -c conda-forge -y
echo "Conda nsis env created"

NSIS_SCRIPT=$THIS_SCRIPT_DIR/project.nsi

# Make windows-like paths because NSIS is weird
SCRIPT_DRIVE_LETTER="$(echo ${COPY_DIR:1:1} | tr [:lower:] [:upper:])"
COPY_DIR=${COPY_DIR////\\}
COPY_DIR="$SCRIPT_DRIVE_LETTER:${COPY_DIR:2}"

NSIS_SCRIPT=${NSIS_SCRIPT////\\}
NSIS_SCRIPT="$SCRIPT_DRIVE_LETTER:${NSIS_SCRIPT:2}"

NSIS_OUTPUT_LOG=$PWD/nsis_log.txt
NSIS_OUTPUT_LOG=${NSIS_OUTPUT_LOG////\\}
NSIS_OUTPUT_LOG="$SCRIPT_DRIVE_LETTER:${NSIS_OUTPUT_LOG:2}"

# Path to makensis from our nsis conda environment
MAKENSIS_COMMAND=$NSIS_CONDA_ENV_PATH/NSIS/makensis
MAKENSIS_COMMAND=${MAKENSIS_COMMAND////\\}
MAKENSIS_COMMAND="$SCRIPT_DRIVE_LETTER:${MAKENSIS_COMMAND:2}"

MANTID_ICON=$THIS_SCRIPT_DIR/../../../images/mantidplot$LOWER_CASE_SUFFIX.ico
MANTID_ICON=${MANTID_ICON////\\}
MANTID_ICON="$SCRIPT_DRIVE_LETTER:${MANTID_ICON:2}"

LICENSE_PATH=$THIS_SCRIPT_DIR/../../../LICENSE.txt
LICENSE_PATH=${LICENSE_PATH////\\}
LICENSE_PATH="$SCRIPT_DRIVE_LETTER:${LICENSE_PATH:2}"

# Generate uninstall commands to make sure to only remove files that are copied by the installer
echo Generating uninstaller helper files
python $THIS_SCRIPT_DIR/create_uninstall_lists.py --package_dir=$COPY_DIR --output_dir=$THIS_SCRIPT_DIR

# Add version info to the package name
VERSION_NAME="$PACKAGE_NAME"-"$VERSION".exe
# Give NSIS full path to the output package name so that it drops it in the current working directory.
OUTFILE_NAME=$PWD/$VERSION_NAME
OUTFILE_NAME=${OUTFILE_NAME////\\}
OUTFILE_NAME="$SCRIPT_DRIVE_LETTER:${OUTFILE_NAME:2}"

# Run the makensis command from our nsis Conda environment
echo makensis /V4 /O\"$NSIS_OUTPUT_LOG\" /DVERSION=$VERSION /DPACKAGE_DIR=\"$COPY_DIR\" /DPACKAGE_SUFFIX=$SUFFIX /DOUTFILE_NAME=$OUTFILE_NAME /DMANTID_ICON=$MANTID_ICON /DMUI_PAGE_LICENSE_PATH=$LICENSE_PATH \"$NSIS_SCRIPT\"
cmd.exe //C "START /wait "" $MAKENSIS_COMMAND /V4 /DVERSION=$VERSION /O"$NSIS_OUTPUT_LOG" /DPACKAGE_DIR="$COPY_DIR" /DPACKAGE_SUFFIX=$SUFFIX /DOUTFILE_NAME=$OUTFILE_NAME /DMANTID_ICON=$MANTID_ICON /DMUI_PAGE_LICENSE_PATH=$LICENSE_PATH "$NSIS_SCRIPT""

if [ ! -f "$OUTFILE_NAME" ]; then
  echo "Error creating package, no file found at $OUTFILE_NAME"
  exit 1
fi

echo "Package packaged, find it here: $OUTFILE_NAME"

echo "Done"
