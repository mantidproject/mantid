#!/usr/bin/env python

from __future__ import (absolute_import, division, print_function)
import optparse
import os
import sys

# set up the command line options
VERSION = "1.1"
THIS_MODULE_DIR = os.path.dirname(os.path.realpath(__file__))
DEFAULT_FRAMEWORK_LOC = os.path.realpath(os.path.join(THIS_MODULE_DIR, "..","lib","systemtests"))
DATA_DIRS_LIST_PATH = os.path.join(THIS_MODULE_DIR, "datasearch-directories.txt")
SAVE_DIR_LIST_PATH = os.path.join(THIS_MODULE_DIR, "defaultsave-directory.txt")

info = []
info.append("This program will configure mantid run all of the system tests located in")
info.append("the 'tests/analysis' directory.")
info.append("This program will create a temporary 'Mantid.user.properties' file which")
info.append("it will rename to 'Mantid.user.properties.systest' upon completion. The")
info.append("current version of the code does not print to stdout while the test is")
info.append("running, so the impatient user may ^C to kill the process. In this case")
info.append("all of the tests that haven't been run will be marked as skipped in the")
info.append("full logs.")


parser = optparse.OptionParser("Usage: %prog [options]", None,
                               optparse.Option, VERSION, 'error', ' '.join(info))
parser.add_option("", "--email", action="store_true",
                  help="send an email with test status.")
parser.add_option("-x", "--executable", dest="executable",
                  help="The executable path used to run each test. Default is the sys.executable")
parser.add_option("-a", "--exec-args", dest="execargs",
                  help="Arguments passed to executable for each test Default=[]")
parser.add_option("", "--frameworkLoc",
                  help="location of the stress test framework (default=%s)" % DEFAULT_FRAMEWORK_LOC)
parser.add_option("", "--disablepropmake", action="store_false", dest="makeprop",
                  help="By default this will move your properties file out of the "
                  + "way and create a new one. This option turns off this behavior.")
parser.add_option("-R", "--tests-regex", dest="testsInclude",
                  help="String specifying which tests to run. Simply uses 'string in testname'.")
parser.add_option("-E", "--excluderegex", dest="testsExclude",
                  help="String specifying which tests to not run. Simply uses 'string in testname'.")
loglevelChoices=["error", "warning", "notice", "information", "debug"]
parser.add_option("-l", "--loglevel", dest="loglevel",
                  choices=loglevelChoices,
                  help="Set the log level for test running: [" + ', '.join(loglevelChoices) + "]")
parser.add_option("", "--showskipped", dest="showskipped", action="store_true",
                  help="List the skipped tests.")
parser.add_option("-d", "--datapaths", dest="datapaths",
                  help="A semicolon-separated list of directories to search for data")
parser.add_option("-s", "--savedir", dest="savedir",
                  help="A directory to use for the Mantid save path")
parser.add_option("", "--archivesearch", dest="archivesearch", action="store_true",
                  help="Turn on archive search for file finder.")
parser.set_defaults(frameworkLoc=DEFAULT_FRAMEWORK_LOC, executable=sys.executable, makeprop=True,
                    loglevel="information")
(options, args) = parser.parse_args()

# import the stress testing framework
sys.path.append(options.frameworkLoc)
import stresstesting

# Configure mantid
# Parse files containing the search and save directories, unless otherwise given
data_paths = options.datapaths
if data_paths is None or data_paths == "":
    with open(DATA_DIRS_LIST_PATH, 'r') as f_handle:
        data_paths = f_handle.read().strip()

save_dir = options.savedir
if save_dir is None or save_dir == "":
    with open(SAVE_DIR_LIST_PATH, 'r') as f_handle:
        save_dir = f_handle.read().strip()
# Configure properties file
mtdconf = stresstesting.MantidFrameworkConfig(loglevel=options.loglevel,
                                              data_dirs=data_paths, save_dir=save_dir,
                                              archivesearch=options.archivesearch)
if options.makeprop:
    mtdconf.config()

# run the tests
execargs = options.execargs
runner = stresstesting.TestRunner(executable=options.executable, exec_args=execargs, escape_quotes=True)
reporter = stresstesting.XmlResultReporter(showSkipped=options.showskipped)
mgr = stresstesting.TestManager(mtdconf.testDir, runner, output = [reporter],
                                testsInclude=options.testsInclude, testsExclude=options.testsExclude)
try:
    mgr.executeTests()
except KeyboardInterrupt:
    mgr.markSkipped("KeyboardInterrupt")

# report the errors
success = reporter.reportStatus()
xml_report = open(os.path.join(mtdconf.saveDir, "TEST-systemtests.xml"),'w')
xml_report.write(reporter.getResults())
xml_report.close()

# put the configuration back to its original state
if options.makeprop:
    mtdconf.restoreconfig()

print()
if mgr.skippedTests == mgr.totalTests:
    print("All tests were skipped")
    success = False # fail if everything was skipped
else:
    percent = 1.-float(mgr.failedTests)/float(mgr.totalTests-mgr.skippedTests)
    percent = int(100. * percent)
    print("%d%s tests passed, %d tests failed out of %d (%d skipped)" %
          (percent, '%', mgr.failedTests, (mgr.totalTests-mgr.skippedTests), mgr.skippedTests))
print('All tests passed? ' + str(success))
if not success:
    sys.exit(1)
