# This script will set up a conda environment before building the doxygen target.
#
# Script usage:
# doxygen.sh <path-to-workspace>
#
# Example command to run a PR build on ubuntu:
# doxygen.sh $WORKSPACE
#
# Expected args:
#   1. WORKSPACE: path to the root of the source code. On Windows, only use / for
#                 this argument do not use \\ or \ in the path.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPT_DIR/mamba-utils

# Check 1 argument is passed and is not optional
if [[ $# < 1 || $1 == "--"* ]]; then
    echo "Pass 1 argument: doxygen.sh <path-to-workspace>"
    exit 1
fi

WORKSPACE=$1
shift 1

cd $WORKSPACE

# Setup Mamba. Create and activate environment
setup_mamba $WORKSPACE/miniforge "" true
create_and_activate_mantid_developer_env $WORKSPACE

# Create the build directory if it doesn't exist
[ -d $WORKSPACE/build ] || mkdir $WORKSPACE/build
cd $WORKSPACE/build

# Set locale to C for latex
export LC_ALL=C

# Configure and generate build files
cmake --preset=doxygen-ci ..

# Build doxygen target
cmake --build . --target doxygen
