import subprocess as sp
import buildNotification
import os

# First build the tests
buildNotification.sendTestBuildStarted("Mantid")

buildlog = open("../../../../logs/Mantid/testsBuild.log","w")
builderr = open("../../../../logs/Mantid/testsBuildErr.log","w")
sp.call("python build.py",stdout=buildlog,stderr=builderr,shell=True)
buildlog.close()
builderr.close()

buildNotification.sendTestBuildCompleted("Mantid")

# Then run them
buildNotification.sendTestStarted("Mantid")

runlog = open("../../../../logs/Mantid/testResults.log","w")
testDir = "Build/Tests"
testsToRun = os.listdir(testDir)
for test in testsToRun:
    if test.endswith("cpp"):
        test = test.split(".")[0]
        runlog.write(test+"\n")
        runlog.flush()
        if os.name == 'nt':
            test += ".exe"
        else:
            test = "./" + test
        sp.call(test,stdout=runlog,shell=True,cwd=testDir)
        runlog.flush()

runlog.close()

