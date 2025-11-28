#!/usr/bin/env bash

build_prefix=$1
prefix=$2
package_name=$3

log_directory=../../../env_logs/$target_platform
mkdir -p $log_directory

#Just for first package (mantid), archive the base environment
if [ "$package_name" == mantid ] ; then
  pixi exec conda list --explicit base > $log_directory/base_environment.txt 2>&1 || echo "Failed to write base conda list output to file"
fi

pixi exec conda list --explicit --prefix "$build_prefix" > $log_directory/"${package_name}"_build_environment.txt 2>&1
pixi exec conda list --explicit --prefix "$prefix" > $log_directory/"${package_name}"_host_environment.txt 2>&1
