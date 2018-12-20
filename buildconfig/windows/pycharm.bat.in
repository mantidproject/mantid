@echo OFF

call %~dp0buildenv.bat

setlocal ENABLEEXTENSIONS
setlocal enabledelayedexpansion

set KEY_NAME="HKCR\Applications\pycharm.exe\shell\open\command"

FOR /F "usebackq tokens=2*" %%A IN (`REG QUERY %KEY_NAME% `) DO (
    set ValueType=%%A
    set ValueValue=%%B
)

if defined ValueValue (
    :: Strip "" and %1
    set ValueValue=!ValueValue:"=!
    set ValueValue=!ValueValue: %%1=!
    start "" "!ValueValue!"
) else (
    @echo %KEY_NAME% not found.
)