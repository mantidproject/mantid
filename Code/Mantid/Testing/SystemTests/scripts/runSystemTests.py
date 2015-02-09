#!/usr/bin/env python

import os
# set up the command line options
VERSION = "1.1"
DEFAULT_FRAMEWORK_LOC = os.path.dirname(os.path.realpath(__file__)) + "/../StressTestFramework"

info = []
info.append("This program will configure mantid run all of the system tests located in")
info.append("the 'SystemTests/AnalysisTests' directory and log the results in 'logs/'.")
info.append("This program will create a temporary 'Mantid.user.properties' file which")
info.append("it will rename to 'Mantid.user.properties.systest' upon completion. The")
info.append("current version of the code does not print to stdout while the test is")
info.append("running, so the impatient user may ^C to kill the process. In this case")
info.append("all of the tests that haven't been run will be marked as skipped in the")
info.append("full logs.")

import optparse
parser = optparse.OptionParser("Usage: %prog [options]", None,
                               optparse.Option, VERSION, 'error', ' '.join(info))
parser.add_option("-m", "--mantidpath", dest="mantidpath",
                  help="Location of mantid build")
parser.add_option("", "--email", action="store_true",
                  help="send an email with test status.")
parser.add_option("", "--frameworkLoc",
		  help="location of the stress test framework (default=%s)" % DEFAULT_FRAMEWORK_LOC)
parser.add_option("", "--disablepropmake", action="store_false", dest="makeprop",
                  help="By default this will move your properties file out of the way and create a new one. This option turns off this behavior.")
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
parser.add_option("", "--archivesearch", dest="archivesearch", action="store_true",
                  help="Turn on archive search for file finder.")
parser.set_defaults(frameworkLoc=DEFAULT_FRAMEWORK_LOC, mantidpath=None, makeprop=True,
                    loglevel="information")
(options, args) = parser.parse_args()

# import the stress testing framework
import sys
import os
import platform
sys.path.append(options.frameworkLoc)
import stresstesting

# Make sure the specified MantidFramework is picked up
# Use specified option if given
mantid_module_path = None
if options.mantidpath is not None:
  mantid_module_path = options.mantidpath
elif os.path.exists("MantidFramework"):
  pass # Current directory is in the already
elif 'MANTIDPATH' in os.environ:
  mantid_module_path = os.environ['MANTIDPATH']
else: 
  pass

# Ensure that this is the one that is picked
sys.path.insert(0, mantid_module_path)

# On Windows & OSX we need to ensure the mantid libraries in bin/Contents/MacOS can be found.
# Must be done before first import of mantid. This is the same as what a user would have to do to
# import mantid in a vanilla python session
# Unfortunately Python seems to know the path separator on each platform but
# not the dynamic library path variable name
if platform.system() == 'Windows':
  path_var = "PATH"
elif platform.system() == 'Darwin':
  path_var = "DYLD_LIBRARY_PATH"
else:
  path_var = None
# Set the path
if path_var:
  os.environ[path_var] = mantid_module_path + os.pathsep + os.environ.get(path_var, "")

# Configure mantid
mtdconf = stresstesting.MantidFrameworkConfig(mantid_module_path, loglevel=options.loglevel,
                                              archivesearch=options.archivesearch)
if options.makeprop:
  mtdconf.config()

# run the tests
reporter = stresstesting.XmlResultReporter(showSkipped=options.showskipped)
mgr = stresstesting.TestManager(mtdconf.testDir, output = [reporter],
                                testsInclude=options.testsInclude, testsExclude=options.testsExclude)
try:
  mgr.executeTests()
except KeyboardInterrupt:
  mgr.markSkipped("KeyboardInterrupt")

# report the errors
success = reporter.reportStatus()
xml_report = open(os.path.join(mtdconf.saveDir, "SystemTestsReport.xml"),'w')
xml_report.write(reporter.getResults())
xml_report.close()

# put the configuratoin back to its original state
if options.makeprop:
  mtdconf.restoreconfig()

print
if mgr.skippedTests == mgr.totalTests:
  print "All tests were skipped"
  success = False # fail if everything was skipped
else:
  percent = 1.-float(mgr.failedTests)/float(mgr.totalTests-mgr.skippedTests)
  percent = int(100. * percent)
  print "%d%s tests passed, %d tests failed out of %d (%d skipped)" % \
      (percent, '%', mgr.failedTests, (mgr.totalTests-mgr.skippedTests), mgr.skippedTests)
print 'All tests passed? ' + str(success)
if not success:
  sys.exit(1)
