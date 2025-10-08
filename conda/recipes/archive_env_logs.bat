@echo on

set build_env_prefix=%1
set host_env_prefix=%2
set package_name=%3

if not exist ..\..\env_logs\win-64 mkdir ..\..\env_logs\win-64
:: Just for first package (mantid), archive the package-conda environment
if %package_name%==mantid call conda list --explicit base > ..\..\env_logs\win-64\base_environment.txt

call conda list --explicit --prefix %host_env_prefix% > ..\..\env_logs\win-64\%package_name%_host_environment.txt
call conda list --explicit --prefix %build_env_prefix% > ..\..\env_logs\win-64\%package_name%_build_environment.txt
