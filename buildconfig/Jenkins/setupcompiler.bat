:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Set up the compiler to use with the mantid build
:: The first argument is the location of the build dir.
::
:: On exit the CM_GENERATOR variable will be set appropriately.
:: If the previous compiler does not match this then the
:: CLEANBUILD flag is also set to yes
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:: Find grep
for /f "delims=" %%I in ('where git') do (
set _git_root_dir=%_git_root_dir:~0,-4%
  @set _grep_exe=%%~dpI..\usr\bin\grep.exe
set _grep_exe=%_git_root_dir%\usr\bin\grep.exe
  @echo Checking for grep at: !_grep_exe!
  if EXIST "!_grep_exe!" (
    goto :endfor
  ) else (
    @set _grep_exe=""
  )
)
:endfor
if !_grep_exe! == "" (
  @echo Unable to find grep.exe
  exit /b 1
)
@echo Using grep: !_grep_exe!

:: SDK 8.1 is backwards compatible with Windows 7. It allows us to target Windows 7
:: when building on newer versions of Windows. This value must be supplied
:: externally and cannot be supplied in the cmake configuration
set _sdk_version=8.1

:: Find the compiler. Try the build tools first and fall back to the full IDE
:: If the tools are not in the standard locations then we give up.
set _vs_version=2017
set _vcvarsall_bat="C:\Program Files (x86)\Microsoft Visual Studio\%_vs_version%\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
if NOT EXIST "!_vcvarsall_bat!" (
  set _vcvarsall_bat="C:\Program Files (x86)\Microsoft Visual Studio\%_vs_version%\Community\VC\Auxiliary\Build\vcvarsall.bat"
  if NOT EXIST "!_vcvarsall_bat!" (
     echo Cannot find vcvarsall.bat from %_vs_version% BuildTools or Community edition. Please ensure one of them is installed in the standard location
    exit /b 1
  )
)
echo Using %_vcvarsall_bat%
call !_vcvarsall_bat! amd64 %_sdk_version%
set CM_GENERATOR=Visual Studio 15 2017 Win64

:: If the previous build was a different generator then do a clean build
set _builddir=%1
if EXIST %_builddir%\CMakeCache.txt (
  call "%_grep_exe%" CMAKE_GENERATOR:INTERNAL %_builddir%\CMakeCache.txt > %_builddir%\cmake_generator.log
  call "%_grep_exe%" -q "%CM_GENERATOR%" %_builddir%\cmake_generator.log
  if ERRORLEVEL 1 (
    set CLEANBUILD=yes
    echo Previous build used a different compiler. Performing a clean build.
  ) else (
    set CLEANBUILD=no
    echo Previous build used the same compiler. No need to clean.
  )
)
