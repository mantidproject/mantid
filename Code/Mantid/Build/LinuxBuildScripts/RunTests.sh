#!/bin/sh
rm ../logs/testsBuild.log
rm ../logs/testsBuildErr.log
rm ../logs/testResults.log
rm ../logs/testsRunErr.log
cd ../checkout/Build/Tests
pwd
sh build.sh >> ../../../logs/testsBuild.log 2> ../../../logs/testsBuildErr.log
python ../LinuxBuildScripts/doTests.py
sh TestsScript.sh 2> ../../../logs/testsRunErr.log
python ../Tests/PythonTests.py
