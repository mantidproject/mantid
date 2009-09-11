import subprocess as sp
import buildNotification
import os

# First build the tests
buildNotification.sendTestBuildStarted("Mantid")

buildlog = open("../../../../logs/Mantid/testsBuild.log","w")
builderr = open("../../../../logs/Mantid/testsBuildErr.log","w")
print "Building the tests..."
sp.call("python build.py",stdout=buildlog,stderr=builderr,shell=True)
print "Tests build complete."
buildlog.close()
builderr.close()

buildNotification.sendTestBuildCompleted("Mantid")

# Then run them
buildNotification.sendTestStarted("Mantid")

print "Running the tests..."
runlog = open("../../../../logs/Mantid/testResults.log","w")
runerr = open("../../../../logs/Mantid/testRunErr.log","w")
testDir = "Build/Tests"
testsToRun = os.listdir(testDir)
for test in testsToRun:
    if test.endswith("cpp"):
        test = test.split(".")[0]
        print "Running " + test
        runlog.write(test+"\n")
        runlog.flush()
        if os.name == 'nt':
            test += ".exe"
        else:
            test = "./" + test
        sp.call(test,stdout=runlog,stderr=runerr,shell=True,cwd=testDir)
        runlog.flush()

runlog.close()
runerr.close()
print "Done running tests."

