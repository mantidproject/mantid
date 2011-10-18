:: This script deals with getting hold of the required third party includes and libraries
:: It will either clone or pull 3rdpartyincludes & 3rdpartylibs-win32/64 and put them in the right place for CMake
:: Your architecture is auto-determined

@echo off

:: Check whether we're 64 or 32 bit. Store it in the 'arch' variable
Set RegQry=HKLM\Hardware\Description\System\CentralProcessor\0
 
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

:: First check if everything is already there - if so we just want to update
IF EXIST Third_Party/include GOTO Update

:: Find out the url where mantid came from so we use the same location & protocol
FOR /F %%I IN ('git.cmd config --get remote.origin.url') DO SET url=%%I
echo %url%
set incs=%url:mantid.git=%3rdpartyincludes.git
echo %incs%
set libs=%url:mantid.git=%3rdpartylibs-%arch%.git
echo %libs%

:: Otherwise we need to clone
echo Cloning Third_Party includes and libraries...
call git.cmd clone %incs% Third_Party/include
call git.cmd clone %libs% Third_Party/lib/%arch%
exit 0


:: Just making sure what we have is up to date
:Update
echo Updating Third_Party includes and libraries...
cd Third_Party/include
call git.cmd pull
cd ../lib/%arch%
call git.cmd pull
:: Be sure to end up back where we started
cd ../../..
exit 0
