:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Set up the compiler to use with the mantid build
:: The first argument is the location of the build dir.
::
:: On exit the CM_GENERATOR variable will be set appropriately.
:: If the previous compiler does not match this then the
:: CLEANBUILD flag is also set to yes
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:: Find grep
for /f "delims=" %%I in ('where git') do @set _git_root_dir=%%~dpI
set _git_root_dir=%_git_root_dir:~0,-4%
set _grep_exe=%_git_root_dir%\usr\bin\grep.exe

:: 8.1 is backwards compatible with Windows 7. It allows us to target Windows 7
:: when building on newer versions of Windows. This value must be supplied
:: externally and cannot be supplied in the cmake configuration
set _sdk_version=8.1
set _vs_version=14
call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64 %_sdk_version%
set CM_GENERATOR=Visual Studio %_vs_version% 2015 Win64

set _builddir=%1
echo %_builddir%
if EXIST %_builddir%\CMakeCache.txt (
  call "%_grep_exe%" CMAKE_LINKER:FILEPATH %_builddir%\CMakeCache.txt > %_builddir%\compiler_version.log
  call "%_grep_exe%" %_vs_version% %_builddir%\compiler_version.log
  if ERRORLEVEL 1 (
    set CLEANBUILD=yes
    echo Previous build used a different compiler. Performing a clean build
  ) else (
    set CLEANBUILD=no
    echo Previous build used the same compiler. No need to clean
  )
)
