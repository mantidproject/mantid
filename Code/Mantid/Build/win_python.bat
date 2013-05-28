:: This is a simple wrapper script to call either python or python_d, passing along the arguments to this script.
:: It is mainly used by CMake in the circumstances where it is not possible to call separate commands in different
:: configurations, Release/Debug. 
::
:: The determination of python vs python_d is simple:
::  - if the directory of the script contains a the file mantid/kernel/kernel.pyd then python.exe is used
::  - if the directory of the script contains a the file mantid/kernel/kernel_d.pyd then python_d.exe is used
@echo off
setlocal

set script_dir=%~dp0
set release_lib=mantid\kernel\_kernel.pyd
set debug_lib=mantid\kernel\_kernel_d.pyd

if EXIST "%script_dir%"%release_lib% (
  python %*
) else (
  if EXIST %script_dir%%debug_lib% (
    python_d %*
  ) else (
    echo No Python libs found. Cannot determine python version to call.
    exit /b 1
  )
)

endlocal