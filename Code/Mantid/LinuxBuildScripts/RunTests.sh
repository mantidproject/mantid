rm ../logs/testsBuild.log
rm ../logs/testsBuildErr.log
rm ../logs/testResults.log
cd ../checkout/Build/Tests
pwd
scons >> ../../../logs/testsBuild.log 2> ../../../logs/testsBuildErr.log
python ../../LinuxBuildScripts/doTests.py

