#!/bin/sh
rm ../../../../logs/testsBuild.log
rm ../../../../logs/testsBuildErr.log
rm ../../../../logs/testResults.log
rm ../../../../logs/testsRunErr.log
rm ../../../../logs/PythonResults.log
sh build.sh >> ../../../../logs/testsBuild.log 2> ../../../../logs/testsBuildErr.log
cd Build/Tests
python ../LinuxBuildScripts/doTests.py
sh TestsScript.sh 2> ../../../../../../logs/testsRunErr.log

