#!/usr/bin/env bash

build_prefix=$1
prefix=$2
package_name=$3

log_directory=../../$target_platform/env_logs

mkdir -p $log_directory
source ../../../mambaforge/etc/profile.d/conda.sh

#Just for first package (mantid), archive the package-conda environment
if [ "$package_name" == mantid ] ; then
  conda list --explicit --prefix ../../../mambaforge/envs/package-conda > $log_directory/package-conda_environment.txt 2>&1 || echo "Failed to write package-conda conda list output to file"
fi
conda list --explicit --prefix "$build_prefix" > $log_directory/"${package_name}"_build_environment.txt 2>&1 || echo "Failed to write build conda list output to file"
conda list --explicit --prefix "$prefix" > $log_directory/"${package_name}"_host_environment.txt 2>&1 || echo "Failed to write host conda list output to file"
