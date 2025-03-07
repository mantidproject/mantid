#!/usr/bin/env bash

build_prefix=$1
prefix=$2
package_name=$3

platform=$(uname)
if [ "$platform" == Linux ] ; then
  platform_dir=linux-64
elif [ "$platform" == Darwin ] ; then
  platform_dir=osx-64
else
  platform_dir=none
fi

mkdir -p ../../$platform_dir/env_logs
source ../../../miniforge/etc/profile.d/conda.sh

#Just for first package (mantid), archive the package-conda environment
if [ "$package_name" == mantid ] ; then
  conda list --explicit --prefix ../../../miniforge/envs/package-conda > ../../$platform_dir/env_logs/package-conda_environment.txt 2>&1 || echo "Failed to write package-conda conda list output to file"
fi
conda list --explicit --prefix "$build_prefix" > ../../$platform_dir/env_logs/"${package_name}"_build_environment.txt 2>&1 || echo "Failed to write build conda list output to file"
conda list --explicit --prefix "$prefix" > ../../$platform_dir/env_logs/"${package_name}"_host_environment.txt 2>&1 || echo "Failed to write host conda list output to file"
