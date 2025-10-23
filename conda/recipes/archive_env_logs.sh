#!/usr/bin/env bash

build_prefix=$1
prefix=$2
package_name=$3

log_directory=../../../env_logs/$target_platform
mkdir -p $log_directory

if [ ! $(command -v conda) ]; then
  if [ -f ../../../miniforge/etc/profile.d/conda.sh ]; then
    source ../../../miniforge/etc/profile.d/conda.sh || exit 1
  else
    echo "Failed to find conda.sh"
    ls ../../../miniforge/
    exit 1
  fi
fi

#Just for first package (mantid), archive the base environment
if [ "$package_name" == mantid ] ; then
  conda list --explicit base > $log_directory/base_environment.txt 2>&1 || echo "Failed to write base conda list output to file"
fi

conda list --explicit --prefix "$build_prefix" > $log_directory/"${package_name}"_build_environment.txt 2>&1
conda list --explicit --prefix "$prefix" > $log_directory/"${package_name}"_host_environment.txt 2>&1
