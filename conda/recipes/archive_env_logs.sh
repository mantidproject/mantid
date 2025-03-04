#!/usr/bin/env bash

build_prefix=$1
prefix=$2
package_name=$3

# This gets the subdir from conda in the form e.g. subdir: linux-64, then extracts
# the platform
platform_dir=$(conda config --show subdir | grep 'subdir' | awk '{print $2}')

mkdir -p ../../$platform_dir/env_logs
source ../../../mambaforge/etc/profile.d/conda.sh

#Just for first package (mantid), archive the package-conda environment
if [ "$package_name" == mantid ] ; then
  conda list --explicit --prefix ../../../mambaforge/envs/package-conda > ../../$platform_dir/env_logs/package-conda_environment.txt 2>&1 || echo "Failed to write package-conda conda list output to file"
fi
conda list --explicit --prefix "$build_prefix" > ../../$platform_dir/env_logs/"${package_name}"_build_environment.txt 2>&1 || echo "Failed to write build conda list output to file"
conda list --explicit --prefix "$prefix" > ../../$platform_dir/env_logs/"${package_name}"_host_environment.txt 2>&1 || echo "Failed to write host conda list output to file"
