@echo off
SETLOCAL ENABLEEXTENSIONS

REM The plugin libraries depend on libraries in the bin directory.  
REM For this session update the dll search path. Note that this is not saved globally
REM and only applies to applications started within this script
SET PATH=%CD%;%PATH%

SET PYTHONSTARTUP=MantidStartup.py
python.exe %1 %2 %3 %4 %5 %6 %7 %8 %9