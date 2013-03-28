@echo off
:: Batch script to install the git macros in to the appropriate location.
:: This assumes you have installed git in the standard location: 
::   32-bit: C:\Program Files\Git
::   64-bit: C:\Program Files(x86)\Git

:: Check whether we're 64 or 32 bit and set the install directory appropriately
set RegQry=HKLM\Hardware\Description\System\CentralProcessor\0
REG.exe Query %RegQry% > checkOS.txt
Find /i "x86" < CheckOS.txt > StringCheck.txt

If %ERRORLEVEL% == 0 (
    set GIT_BIN_DIR="C:\Program Files\Git\bin\"
) ELSE (
    set GIT_BIN_DIR="C:\Program Files (x86)\Git\bin\"
)
:: Remove temporary files created above
del CheckOS.txt
del StringCheck.txt

:: Define files to be copied
set FILES=git-new git-checkbuild gitworkflow-helpers git-finish git-test git-publish

:: Do copy
FOR %%f IN (%FILES%) DO echo Copying %%f to %GIT_BIN_DIR% && xcopy /Y %~dp0%%f %GIT_BIN_DIR%

:: Leave the window open just in case any error messages
pause
