:: This script deals with getting hold of the required third party includes and libraries
:: It will either clone or pull 3rdpartyincludes & 3rdpartylibs-win32/64 and put them in the right place for CMake
::
:: Usage: fetch_Third_Party [win32|win64]
::
::    - [x86|x64] If provided this indicates the required architecture. If left bank your architecture is auto-determined
@echo off

:: Check for git. Older versions used %GitCmd%, newer just git. The older versions still 
:: have git.exe but it should be called through git.cmd so git.cmd check is first
echo Checking for git.cmd
for %%X in (git.cmd) do (set FOUND=%%~$PATH:X)
if defined FOUND (
  set GitCmd=git.cmd
) else (
    echo Not Found. Checking for git.exe
    for %%X in (git.exe) do (set FOUND=%%~$PATH:X)
    if defined FOUND (
        set GitCmd=git.exe
    ) else (
        echo Cannot find git. Make sure the cmd folder is in your path.
        exit /B 1
    )
)
echo Using %GitCmd%

:: It would seem that the %ERRORLEVEL% is not set correctly when inside an if statement
:: so we have to do the check for the OS arch and then override it with the user provided value if necessary

:: Check whether we're 64 or 32 bit. Store it in the 'arch' variable
set RegQry=HKLM\Hardware\Description\System\CentralProcessor\0
 
REG.exe Query %RegQry% > checkOS.txt

Find /i "x86" < CheckOS.txt > StringCheck.txt
 
If %ERRORLEVEL% == 0 (
    set arch=win32
) ELSE (
    set arch=win64
)
:: Remove temporary files created above
del CheckOS.txt
del StringCheck.txt

:: Check if user has overridden the value
if NOT "%1"=="" (
  if "%1"=="win64" (
    set arch=win64
  ) else (
    if "%1"=="win32" (
      set arch=win32
    ) else (
     echo "Unknown architecture. Valid options are:win32,win64."
      exit /B 1
    )
  )
)
echo Using architecture=%arch%


:: Find out the url where mantid came from so we use the same location & protocol
FOR /F %%I IN ('%GitCmd% config --get remote.origin.url') DO SET url=%%I
echo Mantid repository URL: %url%

:: Check if includes are already there - if so, just update
IF EXIST Third_Party/include GOTO UpdateInc

set incs=%url:mantid.git=%3rdpartyincludes.git
:: Otherwise we need to clone
echo Cloning Third_Party includes from %incs%
call %GitCmd% clone %incs% Third_Party/include

:DoLibs
:: Check is libs are already there - if so, just update
IF EXIST Third_Party/lib/%arch% GOTO UpdateLib

set libs=%url:mantid.git=%3rdpartylibs-%arch%.git
echo %libs%

:: Otherwise we need to clone
echo Cloning Third_Party libraries from %libs%
call %GitCmd% clone %libs% Third_Party/lib/%arch%
exit /B 0

:: Just making sure what we have is up to date
:UpdateInc
echo Updating Third_Party includes...
cd Third_Party/include
call %GitCmd% pull
:: Be sure to end up back where we started
cd ../..
GOTO :DoLibs

:UpdateLib
echo Updating Third_Party libraries...
cd Third_Party/lib/%arch%
call %GitCmd% pull
:: Be sure to end up back where we started
cd ../../..
exit /B 0
