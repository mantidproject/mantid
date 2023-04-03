set build_prefix=%1
set prefix=%2
set package_name=%3

if not exist ..\..\win-64\env_logs mkdir ..\..\win-64\env_logs
conda list --explicit --prefix %build_prefix% > ..\..\win-64\env_logs\%package_name%_build_environment.txt
conda list --explicit --prefix %prefix% > ..\..\win-64\env_logs\%package_name%_host_environment.txt
