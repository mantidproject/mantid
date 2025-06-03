#!/usr/bin/env bash

build_prefix=$1
prefix=$2
package_name=$3

lockfile_directory=../../$target_platform/lockfiles
mamba_dir=../../../mambaforge

mkdir -p $lockfile_directory
source $mamba_dir/etc/profile.d/conda.sh

if [ ! -d $mamba_dir/envs/conda-lock ] ; then
  conda create -n conda-lock conda-lock --yes
fi
conda activate $mamba_dir/envs/conda-lock

build_env=$lockfile_directory/"${package_name}"_build_environment.yaml
host_env=$lockfile_directory/"${package_name}"_host_environment.yaml

conda env export --no-builds --prefix "$build_prefix" > $build_env 2>&1 || echo "Failed to create build environment.yaml"
conda env export --no-builds --prefix "$prefix" > $host_env 2>&1 || echo "Failed to create host envionment.yaml"

#change build platform for osx-64 packages due to cross-compilation
if [ "$target_platform" == osx-64 ] ; then
  build_platform=osx-arm64
else
  build_platform=$target_platform
fi

conda-lock --mamba -f $build_env -p $build_platform --lockfile $lockfile_directory/$package_name-build-lockfile.yml 2>&1 || echo "Failed to create build conda lockfile"
conda-lock --mamba -f $host_env -p $target_platform --lockfile $lockfile_directory/$package_name-host-lockfile.yml 2>&1 || echo "Failed to create host conda lockfile"

#clean up
rm $build_env
rm $host_env
conda deactivate
