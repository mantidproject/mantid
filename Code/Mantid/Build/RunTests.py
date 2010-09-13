import subprocess as sp
import buildNotification
import os
import platform
import time

# First build the tests
timeBuildStart = time.time()

log_dir = '../../../../logs/'
buildlog = open(log_dir + "Mantid/testsBuild.log","w")
builderr = open(log_dir + "Mantid/testsBuildErr.log","w")
buildargs=[]
if platform.system() == 'Windows':
    if platform.architecture()[0] == '64bit':
        buildargs.append("win64=1")
elif platform.system() == 'Linux':
    buildargs.append('gcc44=1')
sp.call("python build.py "+' '.join(buildargs),stdout=buildlog,stderr=builderr,shell=True)
buildlog.close()
builderr.close()

timeBuildDone = time.time()

timeTestsBuild = timeBuildDone - timeBuildStart
BuildTimeLog = open(log_dir + "Mantid/timetestbuild.log", "w")
BuildTimeLog.write(str(timeTestsBuild))
BuildTimeLog.close()

# Then run them
timeStart = time.time()

# On the Mac, need to set the path to the shared libraries
# Hopefully can remove this when paths are correctly embedded by build
env_var_name = ''
new_val = os.path.join(os.getcwd(), 'release')
if platform.system() == 'Darwin':
    env_var_name = 'DYLD_LIBRARY_PATH'
    new_val += ':' + os.getcwd()+'/../Third_Party/lib/mac'
elif platform.system() == 'Linux':
    env_var_name = 'LD_LIBRARY_PATH'
else:
    env_var_name = 'PATH'
    new_val += ";" + os.environ[env_var_name]
start_environ = { env_var_name:os.environ[env_var_name] }
os.putenv(env_var_name, new_val)    

runlog = ''
runerr = open(log_dir + "Mantid/testsRunErr.log","w")
testDir = "Build/Tests"
testsToRun = os.listdir(testDir)
for test in testsToRun:
    if test.endswith("cpp"):
        test = test.split(".")[0]
        if not test == 'CurveFitting': continue
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

open(log_dir + "Mantid/testResults.log","w").write(runlog)
runerr.close()

timeFinish = time.time()
timeRun = timeFinish - timeStart
RunTimeLog = open(log_dir + "Mantid/timetestrun.log", "w")
RunTimeLog.write(str(timeRun))
RunTimeLog.close()

# Restore environment
os.putenv(env_var_name,start_environ[env_var_name])