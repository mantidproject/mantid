@echo on

set build_env_prefix=%1
set host_env_prefix=%2
set package_name=%3

if not exist ..\..\..\env_logs\win-64 mkdir ..\..\..\env_logs\win-64
:: Just for first package (mantid), archive the base environment
if %package_name%==mantid call pixi run -e package-standalone conda list --explicit base > ..\..\..\env_logs\win-64\base_environment.txt 2>&1

call pixi run -e package-standalone conda list --explicit --prefix %host_env_prefix% > ..\..\..\env_logs\win-64\%package_name%_host_environment.txt 2>&1
call pixi run -e package-standalone conda list --explicit --prefix %build_env_prefix% > ..\..\..\env_logs\win-64\%package_name%_build_environment.txt 2>&1
