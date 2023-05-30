#!/bin/bash -ex

# This script will set up a conda environment before running the doxygen shell script.
#
# Script usage:
# conda-doxygen <job-name> <path-to-workspace>
#
# Example command to run a PR build on ubuntu:
# conda-doxygen pull_requests-doxygen $WORKSPACE
#
# Expected args:
#   1. JOB_NAME: If the name of the job contains 'pull_requests', cd into the source code directory.
#   2. WORKSPACE: path to the workspace/source code that this should run inside, Windows Caveat: Only use / for
#                 this argument do not use \\ or \ in the path.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPT_DIR/mamba-utils

# Check arguments passed are 2, and aren't optional flags.
if [[ $# < 2 || $1 == "--"* || $2 == "--"* ]]; then
    echo "Pass 2 arguments: conda-doxygen <job-name> <path-to-workspace>"
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

# Mamba
setup_mamba $WORKSPACE/mambaforge "" true
create_and_activate_mantid_developer_env

# Run
$SCRIPT_DIR/doxygen.sh
