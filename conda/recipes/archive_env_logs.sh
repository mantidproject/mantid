#!/usr/bin/env bash
set -ex

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
conda list --explicit --prefix "$build_prefix" 2>&1 ../../$platform_dir/env_logs/"${package_name}"_build_environment.txt || echo "Failed to write build conda list output to file"
conda list --explicit --prefix "$prefix" 2>&1 ../../$platform_dir/env_logs/"${package_name}"_host_environment.txt || echo "Failed to write host conda list output to file"
