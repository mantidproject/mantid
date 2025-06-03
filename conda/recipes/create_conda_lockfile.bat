@echo on

set build_env_prefix=%1
set host_env_prefix=%2
set package_name=%3

set lockfile_dir=..\win-64\lockfiles
set mamba_dir=..\..\mambaforge

if not exist %mamba_dir%\envs\conda-lock call conda create -n conda-lock conda-lock --yes
call conda activate %mamba_dir%\envs\conda-lock

if not exist %lockfile_dir% mkdir %lockfile_dir%

set host_env=%lockfile_dir%\%package_name%_host_environment.yaml
set build_env=%lockfile_dir%\%package_name%_build_environment.yaml

call conda env export --no-builds --prefix %host_env_prefix% > %host_env%
call conda env export --no-builds --prefix %build_env_prefix% > %build_env%

call conda-lock --mamba -f %host_env% -p win-64 --lockfile %package_name%-host-lockfile.yml
call conda-lock --mamba -f %build_env% -p win-64 --lockfile %package_name%-build-lockfile.yml

del %host_env%
del %build_env%
call conda deactivate
