@echo off
python ../scons-local/scons.py %1 %2 %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto err
python ../scons-local/scons.py install %1 %2 %3 %4 %5 %6 %7 %8 %9
exit(0)
:err
pause
