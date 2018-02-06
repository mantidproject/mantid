@echo OFF

call %~dp0buildenv.bat

setlocal ENABLEEXTENSIONS
set KEY_NAME="HKCR\Applications\pycharm.exe\shell\open\command"

FOR /F "usebackq tokens=2*" %%A IN (`REG QUERY %KEY_NAME% `) DO (
    set ValueType=%%A
    set ValueValue=%%B
)

if defined ValueValue (
    @echo Value Type = %ValueType%
    @echo Command = %ValueValue%
    start "" %ValueValue: "%1"=%
) else (
    @echo %KEY_NAME% not found.
)