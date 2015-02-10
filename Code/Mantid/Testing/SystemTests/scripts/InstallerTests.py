"""Finds a package, installs it and runs the tests against it.
"""
import os
import sys
import platform
import shutil
import subprocess
from getopt import getopt

from mantidinstaller import (createScriptLog, log, stop, failure, scriptfailure, 
                             get_installer, run)

THIS_MODULE_DIR = os.path.dirname(os.path.realpath(__file__))
SAVE_DIR_LIST_PATH = os.path.join(THIS_MODULE_DIR, "defaultsave-directory.txt")

try:
    opt, argv = getopt(sys.argv[1:],'d:nohvR:l:')
except:
    opt = [('-h','')]

if ('-h','') in opt:
    print "Usage: %s [OPTIONS]" % os.path.basename(sys.argv[0])
    print
    print "Valid options are:"
    print "       -d Directory to look for packages. Defaults to current working directory"
    print "       -n Run tests without installing Mantid (it must be already installed)"
    print "       -o Output to the screen instead of log files"
    print "       -h Display the usage"
    print "       -R Optionally only run the test matched by the regex"
    print "       -l Log level"
    sys.exit(0)

doInstall = True
test_regex = None
out2stdout = False
log_level = 'notice'
package_dir = os.getcwd()
for option, arg in opt:
    if option == '-n':
        doInstall = False
    if option == '-o':
        out2stdout = True
    if option == '-R' and arg != "":
        test_regex = arg
    if option == '-l' and arg != "":
        log_level = arg
    if option == '-d' and arg != "":
        package_dir = arg

# Log to the configured default save directory
with open(SAVE_DIR_LIST_PATH, 'r') as f_handle:
    output_dir = f_handle.read().strip()

createScriptLog(os.path.join(output_dir, "TestScript.log"))
testRunLogPath = os.path.join(output_dir, "test_output.log")
testRunErrPath = os.path.join(output_dir, "test_errors.log")

log('Starting system tests')
log('Searching for packages in ' + package_dir)
installer = get_installer(package_dir, doInstall)

# Install the found package
if doInstall:
    log("Installing package '%s'" % installer.mantidInstaller)
    try:
        installer.install()
        log("Application path " + installer.mantidPlotPath)
        installer.no_uninstall = False
    except Exception,err:
        scriptfailure("Installing failed. "+str(err))
else:
    installer.no_uninstall = True

# Ensure MANTIDPATH points at this directory so that 
# the correct properties file is loaded              
mantidPlotDir = os.path.dirname(installer.mantidPlotPath)
log('MantidPlot directory %s' % mantidPlotDir)
log('Pointing MANTIDPATH at MantidPlot directory %s' % mantidPlotDir)
os.environ["MANTIDPATH"] = mantidPlotDir

try:
    # Keep hold of the version that was run
    version = run(installer.mantidPlotPath + ' -v')
    version_tested = open('version_tested.log','w')
    if version and len(version) > 0:
        version_tested.write(version)
    version_tested.close()
except Exception, err:
    scriptfailure('Version test failed: '+str(err), installer)

try:
    # Now get the revision number/git commit ID (remove the leading 'g' that isn't part of it)
    revision = run(installer.mantidPlotPath + ' -r').lstrip('g')
    revision_tested = open(os.path.join(output_dir, 'revision_tested.log'), 'w')
    if revision and len(version) > 0:
        revision_tested.write(revision)
    revision_tested.close()
except Exception, err:
    scriptfailure('Revision test failed: '+str(err), installer)

log("Running system tests. Log files are: '%s' and '%s'" % (testRunLogPath,testRunErrPath))
try:
    # Pick the correct Mantid along with the bundled python on windows
    run_test_cmd = "%s %s/runSystemTests.py --loglevel=%s --mantidpath=%s" % \
                (installer.python_cmd, THIS_MODULE_DIR, log_level, mantidPlotDir)
    if test_regex is not None:
        run_test_cmd += " -R " + test_regex
    if out2stdout:
        p = subprocess.Popen(run_test_cmd, shell=True) # no PIPE: print on screen for debugging
        p.wait()
    else:
        p = subprocess.Popen(run_test_cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,shell=True)
        out,err = p.communicate() # waits for p to finish
        testsRunLog = open(testRunLogPath,'w')
        if out:
            testsRunLog.write(out)
        testsRunLog.close()
        testsRunErr = open(testRunErrPath,'w')
        if err:
            testsRunErr.write(err)
        testsRunErr.close()
    if p.returncode != 0:
        failure(installer)
except Exception, exc:
    scriptfailure(str(exc),installer)
except:
    failure(installer)

# Test run completed successfully
stop(installer)
