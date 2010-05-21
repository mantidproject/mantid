import subprocess as sp
import buildNotification
import os
import platform

# First build the tests
buildNotification.sendTestBuildStarted("Mantid")

buildlog = open("../../../../logs/Mantid/testsBuild.log","w")
builderr = open("../../../../logs/Mantid/testsBuildErr.log","w")
buildargs=[]
if platform.system() == 'Windows':
	if platform.architecture()[0] == '64bit':
		buildargs.append("win64=1")
elif platform.system() == 'Linux':
    buildargs.append('gcc44=1')
sp.call("python build.py "+' '.join(buildargs),stdout=buildlog,stderr=builderr,shell=True)
buildlog.close()
builderr.close()

buildNotification.sendTestBuildCompleted("Mantid")

# Then run them
buildNotification.sendTestStarted("Mantid")

# On the Mac, need to set the path to the shared libraries
# Hopefully can remove this when paths are correctly embedded by build
if platform.system() == 'Darwin':
    os.putenv('DYLD_LIBRARY_PATH',os.getcwd()+'/release:'+os.getcwd()+'/../Third_Party/lib/mac')
elif platform.system() == 'Linux':
    os.putenv('LD_LIBRARY_PATH',os.getcwd()+'/release:')
else:
    pass

runlog = ''
runerr = open("../../../../logs/Mantid/testsRunErr.log","w")
testDir = "Build/Tests"
testsToRun = os.listdir(testDir)
for test in testsToRun:
    if test.endswith("cpp"):
        test = test.split(".")[0]
        runlog += test+"\n"
        if os.name == 'posix':
            test = "./" + test
        proc = sp.Popen(test,stdout=sp.PIPE,stderr=runerr,shell=True,cwd=testDir)
        stdout = proc.communicate()[0]
        # The standard out seems to get lost if the process aborted unexpectedly so assume a fatal error occurred.
        if stdout == '':
            runlog += "A fatal error occurred.\n"
        else:
            runlog += stdout
        # Extra bit to help Internet Explorer render this readably
        if os.name == 'posix':
            runlog += "\r\n"

open("../../../../logs/Mantid/testResults.log","w").write(runlog)
runerr.close()

if platform.system() == 'Darwin':
    os.unsetenv('DYLD_LIBRARY_PATH')
elif  platform.system() == 'Linux':
    os.unsetenv('LD_LIBRARY_PATH')
else:
    pass
