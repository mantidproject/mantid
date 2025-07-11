@echo on

set build_env_prefix=%1
set host_env_prefix=%2
set package_name=%3

if not exist ..\win-64\env_logs mkdir ..\win-64\env_logs
:: Just for first package (mantid), archive the package-conda environment
if %package_name%==mantid call conda list --explicit --prefix ..\..\miniforge\envs\package-conda > ..\win-64\env_logs\package-conda_environment.txt

call conda list --explicit --prefix %host_env_prefix% > ..\win-64\env_logs\%package_name%_host_environment.txt
call conda list --explicit --prefix %build_env_prefix% > ..\win-64\env_logs\%package_name%_build_environment.txt
