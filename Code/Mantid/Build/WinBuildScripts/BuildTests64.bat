del ..\..\..\..\logs\Mantid\testsBuild.log
del ..\..\..\..\logs\Mantid\testsBuildErr.log
del ..\..\..\..\logs\Mantid\testResults.log
del ..\..\..\..\logs\Mantid\testsRunErr.log
rem must set to be 64 bit
CALL "%VCINSTALLDIR%"\vcvarsall.bat amd64
build.bat "win64=1" 1>..\..\..\..\logs\Mantid\testsBuild.log 2> ..\..\..\..\logs\Mantid\testsBuildErr.log
