@echo on

set build_env_prefix=%1
set host_env_prefix=%2
set package_name=%3

set lockfile_dir=..\win-64\lockfiles
set mamba_dir=..\..\mambaforge

if not exist %mamba_dir%\envs\conda-lock call conda create -n conda-lock conda-lock --yes
call conda activate %mamba_dir%\envs\conda-lock

if not exist %lockfile_dir% mkdir %lockfile_dir%

set host_env=%lockfile_dir%\%package_name%_host_environment.txt
set build_env=%lockfile_dir%\%package_name%_build_environment.txt

IF %package_name%==mantid (
  set package_conda_env=%lockfile_dir%\package-conda_environment.txt
  call conda env export --no-builds --prefix %mamba_dir%\envs\package-conda > %package_conda_env%
  call conda-lock -f %package_conda_env% -p win-64 --lockfile package-conda-lockfile.yml
  del %package_conda_env%
)

call conda-lock -f %host_env% -p win-64 --lockfile %package_name%-lockfile.yml
call conda-lock -f %build_env% -p win-64 --lockfile %package_name%-lockfile.yml

del %host_env%
del %build_env%
call conda deactivate
