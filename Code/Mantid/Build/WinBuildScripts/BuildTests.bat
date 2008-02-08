del ..\logs\testsBuild.log
del ..\logs\testsBuildErr.log
del ..\logs\testResults.log
del ..\logs\testsRunErr.log
build.bat 1>..\logs\testsBuild.log 2> ..\logs\testsBuildErr.log
