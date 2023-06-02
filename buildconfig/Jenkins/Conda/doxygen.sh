# This script will set up a conda environment before building the doxygen target.
#
# Script usage:
# doxygen.sh <job-name> <path-to-workspace>
#
# Example command to run a PR build on ubuntu:
# doxygen.sh $JOB_NAME $WORKSPACE
#
# Expected args:
#   1. JOB_NAME: If the name of the job contains 'pull_requests', the script enters the workspace directory.
#   2. WORKSPACE: path to the root of the source code. On Windows, only use / for
#                 this argument do not use \\ or \ in the path.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPT_DIR/mamba-utils

# Check 1 argument is passed and is not optional
if [[ $# < 2 || $1 == "--"* || $2 == "--"* ]]; then
    echo "Pass 2 arguments: doxygen.sh <job-name> <path-to-workspace>"
    exit 1
fi

JOB_NAME=$1
WORKSPACE=$2
shift 2

if [[ $JOB_NAME == *pull_requests* ]]; then
    # This relies on the fact pull requests use pull/$PR-NAME
    # which squashes the branch into a single merge commit
    cd $WORKSPACE
fi

# Setup Mamba. Create and activate environment
setup_mamba $WORKSPACE/mambaforge "" true
create_and_activate_mantid_developer_env

# Create the build directory if it doesn't exist
[ -d $WORKSPACE/build ] || mkdir $WORKSPACE/build
cd $WORKSPACE/build

# Set locale to C for latex
export LC_ALL=C

# Configure and generate build files
cmake --preset=doxygen-ci ..

# Build doxygen target
cmake --build . --target doxygen

