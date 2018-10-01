'''
Mantid stress testing framework. This module contains all of the necessary code
to run sets of stress tests on the Mantid framework by executing scripts directly
or by importing them into MantidPlot.

Copyright &copy; 2009 STFC Rutherford Appleton Laboratories

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/systemtests>.
'''
from __future__ import (absolute_import, division, print_function)
from six import PY3
import datetime
import difflib
import imp
import inspect
from mantid.api import FileFinder
from mantid.api import FrameworkManager
from mantid.kernel import config, MemoryStats
from mantid.simpleapi import AlgorithmManager, Load, SaveNexus
import numpy
import os
import platform
import re
import shutil
import subprocess
import shutil
import sys
import tempfile
import time
import unittest


# Path to this file
THIS_MODULE_DIR = os.path.dirname(os.path.realpath(__file__))
# Some windows paths can contain sequences such as \r, e.g. \release_systemtests
# and need escaping to be able to add to the python path
TESTING_FRAMEWORK_DIR = THIS_MODULE_DIR.replace('\\', '\\\\')


#########################################################################
# The base test class.
#########################################################################
class MantidStressTest(unittest.TestCase):
    '''Defines a base class for stress tests, providing functions
    that should be overridden by inheriting classes to perform tests.
    '''

    # Define a delimiter when reporting results
    DELIMITER = '|'

    # Define a prefix for reporting results
    PREFIX = 'RESULT'

    def __init__(self):
        super(MantidStressTest, self).__init__()
        # A list of things not to check when validating
        self.disableChecking = []
        # Whether or not to strip off whitespace when doing simple ascii diff
        self.stripWhitespace = True
        # Tolerance
        self.tolerance = 0.00000001
        # Store the resident memory of the system (in MB) before starting the test
        FrameworkManager.clear()
        self.memory = MemoryStats().residentMem()/1024

    def runTest(self):
        raise NotImplementedError('"runTest(self)" should be overridden in a derived class')

    def skipTests(self):
        '''
        Override this to return True when the tests should be skipped for some
        reason.
        See also: requiredFiles() and requiredMemoryMB()
        '''
        return False

    def excludeInPullRequests(self):
        '''
        Override this to return True if the test is too slow or deemed unnecessary to
        be run with every pull request. These tests will be run nightly instead.
        '''
        return False

    def validate(self):
        '''
        Override this to provide a pair of workspaces which should be checked for equality
        by the doValidation method.
        The overriding method should return a pair of strings. This could be two workspace
        names, e.g. return 'workspace1','workspace2', or a workspace name and a nexus
        filename (which must have nxs suffix), e.g. return 'workspace1','GEM00001.nxs'.
        '''
        return None

    def requiredFiles(self):
        '''
        Override this method if you want to require files for the test.
        Return a list of files.
        '''
        return []

    def requiredMemoryMB(self):
        '''
        Override this method to specify the amount of free memory,
        in megabytes, that is required to run the test.
        The test is skipped if there is not enough memory.
        '''
        return 0

    def validateMethod(self):
        '''
        Override this to specify which validation method to use. Look at the validate* methods to
        see what allowed values are.
        '''
        return "WorkspaceToNeXus"

    def maxIterations(self):
        '''Override this to perform more than 1 iteration of the implemented test.'''
        return 1

    def reportResult(self, name, value):
        '''
        Send a result to be stored as a name,value pair
        '''
        output = self.PREFIX + self.DELIMITER + name + self.DELIMITER + str(value) + "\n"
        # Ensure that this is all printed together and not mixed with stderr
        sys.stdout.flush()
        sys.stdout.write(output)
        sys.stdout.flush()

    def __verifyRequiredFile(self, filename):
        '''Return True if the specified file name is findable by Mantid.'''
        # simple way is just getFullPath which never uses archive search
        if os.path.exists(FileFinder.getFullPath(filename)):
            return True

        # try full findRuns which will use archive search if it is turned on
        try:
            candidates = FileFinder.findRuns(filename)
            for item in candidates:
                if os.path.exists(item):
                    return True
        except RuntimeError:
            return False

        # file was not found
        return False

    def __verifyRequiredFiles(self):
        # first see if there is anything to do
        reqFiles = self.requiredFiles()
        if len(reqFiles) <= 0:
            return

        # by default everything is ok
        foundAll = True

        # check that all of the files exist
        for filename in reqFiles:
            if not self.__verifyRequiredFile(filename):
                print("Missing required file: '%s'" % filename)
                foundAll = False

        if not foundAll:
            sys.exit(TestRunner.SKIP_TEST)

    def __verifyMemory(self):
        """Do we need to skip due to lack of memory?"""
        required = self.requiredMemoryMB()
        if required <= 0:
            return

        # Check if memory is available
        MB_avail = MemoryStats().availMem()/(1024.)
        if (MB_avail < required):
            print("Insufficient memory available to run test! %g MB available, need %g MB." % (MB_avail, required))
            sys.exit(TestRunner.SKIP_TEST)

    def execute(self):
        '''Run the defined number of iterations of this test'''
        # Do we need to skip due to missing files?
        self.__verifyRequiredFiles()

        self.__verifyMemory()

        # A custom check for skipping the tests for other reasons
        if self.skipTests():
            sys.exit(TestRunner.SKIP_TEST)

        # A custom check for skipping tests that shouldn't be run with every PR
        if self.excludeInPullRequests():
            sys.exit(TestRunner.SKIP_TEST)

        # Start timer
        start = time.time()
        countmax = self.maxIterations() + 1
        for i in range(1, countmax):
            istart = time.time()
            self.runTest()
            delta_t = time.time() - istart
            self.reportResult('iteration time_taken', str(i) + ' %.2f' % delta_t)
        delta_t = float(time.time() - start)
        # Finish
        self.reportResult('time_taken', '%.2f' % delta_t)

    def __prepASCIIFile(self, filename):
        """Prepare an ascii file for comparison using difflib."""
        with open(filename, mode='r') as handle:
            stuff = handle.readlines()

        if self.stripWhitespace:
            stuff = [line.strip() for line in stuff]

        return stuff

    def validateASCII(self):
        """Validate ASCII files using difflib."""
        (measured, expected) = self.validate()
        if not os.path.isabs(measured):
            measured = FileFinder.Instance().getFullPath(measured)
        if not os.path.isabs(expected):
            expected = FileFinder.Instance().getFullPath(expected)

        measured = self.__prepASCIIFile(measured)
        expected = self.__prepASCIIFile(expected)

        # calculate the difference
        diff = difflib.Differ().compare(measured, expected)
        result = []
        for line in diff:
            if line.startswith('+') or line.startswith('-') or line.startswith('?'):
                result.append(line)

        # print the difference
        if len(result) > 0:
            if self.stripWhitespace:
                msg = "(whitespace striped from ends)"
            else:
                msg = ""
            print("******************* Difference in files", msg)
            print("\n".join(result))
            print("*******************")
            return False
        else:
            return True

    def validateWorkspaceToNeXus(self):
        '''
        Assumes the second item from self.validate() is a nexus file and loads it
        to compare to the supplied workspace.
        '''
        valNames = list(self.validate())
        numRezToCheck = len(valNames)
        mismatchName = None

        validationResult = True
        # results are in pairs
        for valname, refname in zip(valNames[::2], valNames[1::2]):
            if refname.endswith('.nxs'):
                Load(Filename=refname, OutputWorkspace="RefFile")
                refname = "RefFile"
            else:
                raise RuntimeError("Should supply a NeXus file: %s" % refname)
            valPair = (valname, "RefFile")
            if numRezToCheck > 2:
                mismatchName = valname

            if not(self.validateWorkspaces(valPair, mismatchName)):
                validationResult = False
                print('Workspace {0} not equal to its reference file'.format(valname))

        return validationResult

    def validateWorkspaceToWorkspace(self):
        '''
        Assumes the second item from self.validate() is an existing workspace
        to compare to the supplied workspace.
        '''
        valNames = list(self.validate())
        return self.validateWorkspaces(valNames)

    def validateWorkspaces(self, valNames=None, mismatchName=None):
        '''
        Performs a check that two workspaces are equal using the CompareWorkspaces
        algorithm. Loads one workspace from a nexus file if appropriate.
        Returns true if: the workspaces match
                      OR the validate method has not been overridden.
        Returns false if the workspace do not match. The reason will be in the log.
        '''
        if valNames is None:
            valNames = self.validate()

        checker = AlgorithmManager.create("CompareWorkspaces")
        checker.setLogging(True)
        checker.setPropertyValue("Workspace1", valNames[0])
        checker.setPropertyValue("Workspace2", valNames[1])
        checker.setProperty("Tolerance", float(self.tolerance))
        if hasattr(self, 'tolerance_is_reller') and self.tolerance_is_reller:
            checker.setProperty("ToleranceRelerr", True)
        for d in self.disableChecking:
            checker.setProperty("Check"+d, False)
        checker.execute()
        if not checker.getProperty("Result").value:
            print(self.__class__.__name__)
            if mismatchName:
                SaveNexus(InputWorkspace=valNames[0],
                          Filename=self.__class__.__name__+mismatchName+'-mismatch.nxs')
            else:
                SaveNexus(InputWorkspace=valNames[0],
                          Filename=self.__class__.__name__+'-mismatch.nxs')
            return False

        return True

    def doValidation(self):
        """
        Perform validation. This selects which validation method to use by the result
        of validateMethod() and validate(). If validate() is not overridden this will
        return True.
        """
        # if no validation is specified then it must be ok
        validation = self.validate()
        if validation is None:
            return True

        # if a simple boolean then use this
        if type(validation) == bool:
            return validation
        # or numpy boolean
        if type(validation) == numpy.bool_:
            return bool(validation)

        # switch based on validation methods
        method = self.validateMethod()
        if method is None:
            return True  # don't validate

        method = method.lower()
        if "validateworkspacetonexus".endswith(method):
            return self.validateWorkspaceToNeXus()
        elif "validateworkspacetoworkspace".endswith(method):
            return self.validateWorkspaceToWorkspace()
        elif "validateascii".endswith(method):
            return self.validateASCII()
        else:
            raise RuntimeError("invalid validation method '%s'" % self.validateMethod())

    def returnValidationCode(self, code):
        """
        Calls doValidation() and returns 0 in success and code if failed. This will be
        used as return code from the calling python subprocess
        """
        if self.doValidation():
            retcode = 0
        else:
            retcode = code
        if retcode == 0:
            self._success = True
        else:
            self._success = False
        # Now the validation is complete we can clear out all the stored data and check memory usage
        FrameworkManager.clear()
        # Get the resident memory again and work out how much it's gone up by (in MB)
        memorySwallowed = MemoryStats().residentMem()/1024 - self.memory
        # Store the result
        self.reportResult('memory footprint increase', memorySwallowed)
        return retcode

    def succeeded(self):
        """Returns true if the test has been run and it succeeded, false otherwise"""
        if hasattr(self, '_success'):
            return self._success
        else:
            return False

    def cleanup(self):
        '''
        This function is called after a test has completed and can be used to
        clean up, i.e. remove workspaces etc
        '''
        pass

    def assertDelta(self, value, expected, delta, msg=""):
        """Check that a value is within +- delta of the expected value"""
        # Build the error message
        if len(msg) > 0:
            msg += " "
        msg += "Expected %g == %g within +- %g." % (value, expected, delta)

        if (value > expected+delta) or (value < expected-delta):
            raise Exception(msg)

    def assertLessThan(self, value, expected, msg=""):
        """
        Check that a value is < expected.
        """
        # Build the error message
        if len(msg) > 0:
            msg += " "
        msg += "Expected %g < %g " % (value, expected)

        if (value >= expected):
            raise Exception(msg)

    def assertGreaterThan(self, value, expected, msg=""):
        """
        Check that a value is > expected.
        """
        # Build the error message
        if len(msg) > 0:
            msg += " "
        msg += "Expected %g > %g " % (value, expected)

        if (value <= expected):
            raise Exception(msg)

    def assertRaises(self, excClass, callableObj=None, *args, **kwargs):
        """
        Check that a callable raises an exception when called.
        """
        was_raised = True
        try:
            callableObj(*args, **kwargs)
            was_raised = False
        except excClass:
            pass
        except Exception as e:
            msg = 'Expected {0} but raised {1} instead.'
            raise Exception(msg.format(excClass.__name__, e.__class__.__name__))

        if not was_raised:
            raise Exception('{} not raised'.format(excClass.__name__))


#########################################################################
# A class to store the results of a test
#########################################################################
class TestResult(object):
    '''
    Stores the results of each test so that they can be reported later.
    '''

    def __init__(self):
        self._results = []
        self.name = ''
        self.filename = ''
        self.date = ''
        self.status = ''
        self.time_taken = ''
        self.total_time = ''
        self.output = ''
        self.err = ''

    def __eq__(self, other):
        return self.name == other.name

    def __lt__(self, other):
        return self.name < other.name

    def addItem(self, item):
        '''
        Add an item to the store, this should be a list containing 2 entries: [Name, Value]
        '''
        self._results.append(item)

    def resultLogs(self):
        '''
        Get the map storing the results
        '''
        return self._results


#########################################################################
# A base class to support report results in an appropriate manner
#########################################################################
class ResultReporter(object):
    '''
    A base class for results reporting. In order to get the results in an
    appropriate form, subclass this class and implement the dispatchResults
    method.
    '''

    def __init__(self, total_number_of_tests=0, maximum_name_length=0):
        '''Initialize a class instance, e.g. connect to a database'''
        self._total_number_of_tests = total_number_of_tests
        self._maximum_name_length = maximum_name_length
        pass

    def dispatchResults(self, result, number_of_completed_tests):
        raise NotImplementedError('"dispatchResults(self, result)" should be overridden in a derived class')

    def printResultsToConsole(self, result, number_of_completed_tests):
        '''
        Print the results to standard out
        '''
        if ((result.status == 'skipped') and (not self._show_skipped)):
            pass
        else:
            console_output = ''
            if self._quiet:
                percentage = int(float(number_of_completed_tests)*100.0/float(self._total_number_of_tests))
                if len(result._results) < 6:
                    time_taken = " -- "
                else:
                    time_taken = result._results[6][1]
                console_output += '[{:>3d}%] {:>3d}/{:>3d} : '.format(percentage, number_of_completed_tests,
                                                                      self._total_number_of_tests)
                console_output += '{:.<{}} ({}: {}s)'.format(result.name+" ", self._maximum_name_length+2,
                                                             result.status, time_taken)
            if ((self._output_on_failure
                and (result.status != 'success')
                and (result.status != 'skipped'))
                or (not self._quiet)):
                nstars = 80
                console_output += '\n' + ('*' * nstars) + '\n'
                print_list = ['test_name', 'filename', 'test_date', 'host_name', 'environment',
                              'status', 'time_taken', 'memory footprint increase', 'output', 'err']
                for key in print_list:
                    key_not_found = True
                    for i in range(len(result._results)):
                        if key == result._results[i][0]:
                            console_output += '{}: {}\n'.format(key, result._results[i][1])
                            key_not_found = False
                    if key_not_found:
                        try:
                            console_output += '{}: {}\n'.format(key, getattr(result, key))
                        except AttributeError:
                            pass
                console_output += ('*' * nstars) + '\n'
            print(console_output)
            sys.stdout.flush()
        return


#########################################################################
# A class to report results as formatted text output
#########################################################################
class TextResultReporter(ResultReporter):
    '''
    Report the results of a test using standard out
    '''

    def dispatchResults(self, result, number_of_completed_tests):
        '''
        The default text reporter prints to standard out
        '''
        self.printResultsToConsole(result, number_of_completed_tests)
        return

# A class to report results as junit xml
# DO NOT MOVE
from xmlreporter import XmlResultReporter  # noqa


#########################################################################
# A base class for a TestRunner
#########################################################################
class TestRunner(object):
    '''
    A base class to serve as a wrapper to actually run the tests in a specific
    environment, i.e. console, gui
    '''
    SUCCESS_CODE = 0
    GENERIC_FAIL_CODE = 1
    SEGFAULT_CODE = 139
    VALIDATION_FAIL_CODE = 99
    NOT_A_TEST = 98
    SKIP_TEST = 97

    def __init__(self, executable, exec_args=None, escape_quotes=False, clean=False):
        self._executable = executable
        self._exec_args = exec_args
        self._test_dir = ''
        self._escape_quotes = escape_quotes
        self._clean = clean

    def getTestDir(self):
        return self._test_dir

    def setTestDir(self, test_dir):
        self._test_dir = os.path.abspath(test_dir).replace('\\', '/')

    def spawnSubProcess(self, cmd):
        '''Spawn a new process and run the given command within it'''
        proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, bufsize=-1)
        std_out, _ = proc.communicate()
        return proc.returncode, std_out

    def start(self, script):
        '''Run the given test code in a new subprocess'''
        exec_call = self._executable
        if self._exec_args:
            exec_call += ' ' + self._exec_args
        # write script to temporary file and execute this file
        tmp_file = tempfile.NamedTemporaryFile(mode='w', delete=False)
        tmp_file.write(script.asString(clean=self._clean))
        tmp_file.close()
        cmd = exec_call + ' ' + tmp_file.name
        results = self.spawnSubProcess(cmd)
        os.remove(tmp_file.name)
        return results


#########################################################################
# Encapsulate the script for running a single test
#########################################################################
class TestScript(object):

    def __init__(self, test_dir, module_name, test_cls_name, exclude_in_pr_builds):
        self._test_dir = test_dir
        self._modname = module_name
        self._test_cls_name = test_cls_name
        self._exclude_in_pr_builds = not exclude_in_pr_builds

    def asString(self, clean=False):
        code = "import sys\n" + \
               ("sys.path.append('%s')\n" % TESTING_FRAMEWORK_DIR) + \
               ("sys.path.append('%s')\n" % self._test_dir) + \
               ("from %s import %s\n" % (self._modname, self._test_cls_name)) + \
               ("systest = %s()\n" % self._test_cls_name) + \
               ("if %r:\n" % self._exclude_in_pr_builds) + \
               ("    systest.excludeInPullRequests = lambda: False\n")
        if (not clean):
            code += "systest.execute()\n" + \
                    ("exitcode = systest.returnValidationCode(%i)\n" % TestRunner.VALIDATION_FAIL_CODE)
        else:
            code += "exitcode = 0\n"
        code += "systest.cleanup()\nsys.exit(exitcode)\n"
        return code


#########################################################################
# A class to tie together a test and its results
#########################################################################
class TestSuite(object):
    '''
    Tie together a test and its results.
    '''
    def __init__(self, test_dir, modname, testname, filename=None):
        self._test_dir = test_dir
        self._modname = modname
        self._test_cls_name = testname
        self._fqtestname = modname

        # A None testname indicates the source did not load properly
        # It has come this far so that it gets reported as a proper failure
        # by the framework
        if testname is not None:
            self._fqtestname += '.' + testname

        self._result = TestResult()
        # Add some results that are not linked to the actually test itself
        self._result.name = self._fqtestname
        if filename:
            self._result.filename = filename
        else:
            self._result.filename = self._fqtestname
        self._result.addItem(['test_name', self._fqtestname])
        sysinfo = platform.uname()
        self._result.addItem(['host_name', sysinfo[1]])
        self._result.addItem(['environment', self.envAsString()])
        self._result.status = 'skipped'  # the test has been skipped until it has been executed

    name = property(lambda self: self._fqtestname)
    status = property(lambda self: self._result.status)

    def envAsString(self):
        if os.name == 'nt':
            system = platform.system().lower()[:3]
            arch = platform.architecture()[0][:2]
            env = system + arch
        elif os.name == 'mac':
            env = platform.mac_ver()[0]
        else:
            env = platform.dist()[0]
        return env

    def markAsSkipped(self, reason):
        self.setOutputMsg(reason)
        self._result.status = 'skipped'

    def execute(self, runner, exclude_in_pr_builds):
        if self._test_cls_name is not None:
            script = TestScript(self._test_dir, self._modname, self._test_cls_name, exclude_in_pr_builds)
            # Start the new process and wait until it finishes
            retcode, output = runner.start(script)
        else:
            retcode, output = TestRunner.SKIP_TEST, ""

        self._result.date = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        self._result.addItem(['test_date', self._result.date])

        if retcode == TestRunner.SUCCESS_CODE:
            status = 'success'
        elif retcode == TestRunner.GENERIC_FAIL_CODE:
            # This is most likely an algorithm failure, but it's not certain
            status = 'algorithm failure'
        elif retcode == TestRunner.VALIDATION_FAIL_CODE:
            status = 'failed validation'
        elif retcode == TestRunner.SEGFAULT_CODE:
            status = 'crashed'
        elif retcode == TestRunner.SKIP_TEST:
            status = 'skipped'
        elif retcode < 0:
            status = 'hung'
        else:
            status = 'unknown'

        # Check return code and add result
        self._result.status = status
        self._result.addItem(['status', status])
        # Dump std out so we know what happened
        if PY3:
            if isinstance(output, bytes):
                output = output.decode()
        self._result.output = '\n' + output
        all_lines = output.split('\n')
        # Find the test results
        for line in all_lines:
            entries = line.split(MantidStressTest.DELIMITER)
            if len(entries) == 3 and entries[0] == MantidStressTest.PREFIX:
                self._result.addItem([entries[1], entries[2]])

    def setOutputMsg(self, msg=None):
        if msg is not None:
            self._result.output = msg

    def reportResults(self, reporters, number_of_completed_tests):
        for r in reporters:
            r.dispatchResults(self._result, number_of_completed_tests)


#########################################################################
# The main API class
#########################################################################
class TestManager(object):
    '''A manager class that is responsible for overseeing the testing process.
    This is the main interaction point for the framework.
    '''

    def __init__(self, test_loc=None, runner=None, output=[TextResultReporter()],
                 quiet=False, testsInclude=None, testsExclude=None, showSkipped=False,
                 exclude_in_pr_builds=None, output_on_failure=False, clean=False,
                 process_number=0, ncores=1, list_of_tests=None):
        '''Initialize a class instance'''

        # Runners and reporters
        self._runner = runner
        self._reporters = output
        for r in self._reporters:
            r._quiet = quiet
            r._output_on_failure = output_on_failure
        self._clean = clean
        self._showSkipped = showSkipped

        self._testDir = test_loc
        self._quiet = quiet
        self._testsInclude=testsInclude
        self._testsExclude=testsExclude
        self._exclude_in_pr_builds=exclude_in_pr_builds

        self._passedTests = 0
        self._skippedTests = 0
        self._failedTests = 0
        self._lastTestRun = 0

        self._tests = list_of_tests

    def generateMasterTestList(self):

        # If given option is a directory
        if os.path.isdir(self._testDir) == True:
            test_dir = os.path.abspath(self._testDir).replace('\\', '/')
            sys.path.append(test_dir)
            self._runner.setTestDir(test_dir)
            full_test_list = self.loadTestsFromDir(test_dir)
        else:
            if os.path.exists(self._testDir) == False:
                print('Cannot find file ' + self._testDir + '.py. Please check the path.')
                exit(2)
            test_dir = os.path.abspath(os.path.dirname(self._testDir)).replace('\\', '/')
            sys.path.append(test_dir)
            self._runner.setTestDir(test_dir)
            full_test_list = self.loadTestsFromModule(os.path.basename(self._testDir))

        # Gather statistics on full test list
        test_stats = [0, 0, 0]
        test_stats[2] = len(full_test_list)
        reduced_test_list = []
        for t in full_test_list:
            if self.__shouldTest(t) or self._showSkipped:
                reduced_test_list.append(t)

        if len(reduced_test_list) == 0:
            print('No tests defined in ' + test_dir +
                  '. Please ensure all test classes sub class stresstesting.MantidStressTest.')
            exit(2)

        test_stats[0] = len(reduced_test_list)
        for t in reduced_test_list:
            test_stats[1] = max(test_stats[1], len(t._fqtestname))

        # When using multiprocessing, we have to split the list of tests among
        # the processes into groups instead of test by test, to avoid issues
        # with data being cleaned up before another process has finished.
        #
        # We create a list of test modules (= different python files in the
        # 'Testing/SystemTests/tests/analysis' directory) and count how many
        # tests are in each module. We also create on the fly a list of tests
        # for each module.
        modcounts = dict()
        modtests = dict()
        for t in reduced_test_list:
            key = t._modname
            if key in modcounts.keys():
                modcounts[key] += 1
                modtests[key].append(t)
            else:
                modcounts[key] = 1
                modtests[key] = [t]

        # Now we scan each test module (= python file) and list all the data files
        # that are used by that module. The possible ways files are being specified
        # are:
        # 1. if the extension '.nxs' is present in the line
        # 2. if there is a sequence of at least 4 digits inside a string
        # In case number 2, we have to search for strings starting with 4 digits,
        # i.e. "0123, or strings ending with 4 digits 0123".
        # This might over-count, meaning some sequences of 4 digits might not be
        # used for a file name specification, but it does not matter if it gets
        # identified as a filename as the probability of the same sequence being
        # present in another python file is small, and it would therefore not lock
        # any other tests.

        # Some dictionaries to store the info
        files_required_by_test_module = dict()
        data_file_lock_status = dict()
        # The extension most commonly used
        extensions = [".nxs", ".raw", ".RAW"]
        # A regex check is used to iterate back from the position of '.nxs' and
        # check that the current character is still part of a variable name. This
        # is needed to find the start of the string, hence the total filename.
        check = re.compile("[A-Za-z0-9_-]")
        # In the case of looking for digits inside strings, the strings can start
        # with either " or '
        string_quotation_mark = ["'",'"']

        # Now look through all the test modules and build the list of data files
        for modkey in modtests.keys():

            fname = modkey+".py"
            files_required_by_test_module[modkey] = []
            with open(os.path.join(os.path.dirname(self._testDir), "analysis", fname),"r") as pyfile:
                for line in pyfile.readlines():

                    # Search for all instances of '.nxs' or '.raw'
                    for ext in extensions:
                        for indx in [m.start() for m in re.finditer(ext, line)]:
                            # When '.nxs' is found, iterate backwards to find the start
                            # of the filename.
                            for i in range(indx-1,1,-1):
                                # If the present character is not either a letter, digit,
                                # underscore, or hyphen then the beginning of the filename
                                # has been found
                                if not check.search(line[i]):
                                    key = line[i+1:indx]+ext
                                    if (key not in files_required_by_test_module[modkey]) and (key != ext):
                                        files_required_by_test_module[modkey].append(key)
                                        data_file_lock_status[key] = False
                                    break

                    # Search for '0123 or "0123
                    for so in string_quotation_mark:
                        p = re.compile(so+r"\d{4}")
                        for m in p.finditer(line):
                            # Iterate forwards to find the closing quotation mark
                            for i in range(m.end(),len(line)):
                                if line[i] == so:
                                    key = line[m.start()+1:i]
                                    if key not in files_required_by_test_module[modkey]:
                                        files_required_by_test_module[modkey].append(key)
                                        data_file_lock_status[key] = False
                                    break

                    # Search for 0123' or 0123"
                    for so in string_quotation_mark:
                        p = re.compile(r"\d{4}"+so)
                        for m in p.finditer(line):
                            # Iterate backwards to find the opening quotation mark
                            for i in range(m.start(),1,-1):
                                if line[i] == so:
                                    key = line[i+1:m.end()-1]
                                    if key not in files_required_by_test_module[modkey]:
                                        files_required_by_test_module[modkey].append(key)
                                        data_file_lock_status[key] = False
                                    break

        if (not self._quiet):
            for key in files_required_by_test_module.keys():
                print('=' * 45)
                print(key)
                for s in files_required_by_test_module[key]:
                    print(s)

        return modcounts, modtests, test_stats, files_required_by_test_module, data_file_lock_status

    def __shouldTest(self, suite):
        if self._testsInclude is not None:
            if self._testsInclude not in suite.name:
                suite.markAsSkipped("NotIncludedTest")
                return False
        if self._testsExclude is not None:
            if self._testsExclude in suite.name:
                suite.markAsSkipped("ExcludedTest")
                return False
        return True

    def executeTests(self, tests_done=None):
        # Get the defined tests
        for suite in self._tests:
            if self.__shouldTest(suite):
                suite.execute(self._runner, self._exclude_in_pr_builds)
            if suite.status == "success":
                self._passedTests += 1
            elif suite.status == "skipped":
                self._skippedTests += 1
            else:
                self._failedTests += 1
            with tests_done.get_lock():
                tests_done.value += 1
            if not self._clean:
                suite.reportResults(self._reporters, tests_done.value)
            self._lastTestRun += 1

    def markSkipped(self, reason=None, tests_done_value=0):
        for suite in self._tests[self._lastTestRun:]:
            suite.setOutputMsg(reason)
            # Just let people know you were skipped
            suite.reportResults(self._reporters, tests_done_value)

    def loadTestsFromDir(self, test_dir):
        ''' Load all of the tests defined in the given directory'''
        entries = os.listdir(test_dir)
        tests = []
        regex = re.compile('^.*\.py$', re.IGNORECASE)
        for file in entries:
            if regex.match(file) != None:
                tests.extend(self.loadTestsFromModule(os.path.join(test_dir, file)))
        return tests

    def loadTestsFromModule(self, filename):
        '''
        Load test classes from the given module object which has been
        imported with the __import__ statement
        '''
        modname = os.path.basename(filename)
        modname = modname.split('.py')[0]
        tests = []
        try:
            with open(filename, 'r') as pyfile:
                mod = imp.load_module(modname, pyfile, filename, ("", "", imp.PY_SOURCE))
                mod_attrs = dir(mod)
                for key in mod_attrs:
                    value = getattr(mod, key)
                    if key is "MantidStressTest" or not inspect.isclass(value):
                        continue
                    if self.isValidTestClass(value):
                        test_name = key
                        tests.append(TestSuite(self._runner.getTestDir(), modname, test_name, filename))
        except Exception as exc:
            print("Error importing module '%s': %s" % (modname, str(exc)))
            # Error loading the source, add fake unnamed test so that an error
            # will get generated when the tests are run and it will be counted properly
            tests.append(TestSuite(self._runner.getTestDir(), modname, None, filename))
        finally:
            pyfile.close()
        return tests

    def isValidTestClass(self, class_obj):
        """Returns true if the test is a valid test class. It is valid
        if: the class subclassses MantidStressTest and has no abstract methods
        """
        if not issubclass(class_obj, MantidStressTest):
            return False
        # Check if the get_reference_file is abstract or not
        if hasattr(class_obj, "__abstractmethods__"):
            if len(class_obj.__abstractmethods__) == 0:
                return True
            else:
                return False
        else:
            return True


#########################################################################
# Class to handle the environment
#########################################################################
class MantidFrameworkConfig:

    def __init__(self, sourceDir=None,
                 data_dirs="", save_dir="",
                 loglevel='information', archivesearch=False):
        self.__sourceDir = self.__locateSourceDir(sourceDir)

        # add location of stress tests
        self.__testDir = self.__locateTestsDir()

        # add location of the analysis tests
        sys.path.insert(0, self.__locateTestsDir())

        # setup the rest of the magic directories
        self.__saveDir = save_dir
        if not os.path.exists(save_dir):
            print("Making directory %s to save results" % save_dir)
            os.mkdir(save_dir)

        else:
            if not os.path.isdir(save_dir):
                raise RuntimeError("%s is not a directory" % save_dir)

        # assume a string is already semicolon-seaprated
        if type(data_dirs) == str:
            self.__dataDirs = data_dirs
            self.__dataDirs += ";%s" % self.__saveDir
        else:
            data_path = ""
            data_dirs.append(self.__saveDir)
            for direc in data_dirs:
                if not os.path.exists(direc):
                    raise RuntimeError('Directory ' + direc + ' was not found.')
                search_dir = direc.replace('\\', '/')
                if not search_dir.endswith('/'):
                    search_dir += '/'
                    data_path += search_dir + ';'

            self.__dataDirs = data_path

        # set the log level
        self.__loglevel = loglevel
        self.__datasearch = archivesearch

    def __locateSourceDir(self, suggestion):
        if suggestion is None:
            loc = os.path.abspath(__file__)
            suggestion = os.path.split(loc)[0]  # get the directory
        loc = os.path.abspath(suggestion)
        loc = os.path.normpath(loc)

        if os.path.isdir(loc):
            return loc
        else:
            raise RuntimeError("Failed to find source directory")

    def __locateTestsDir(self):
        loc = os.path.join(self.__sourceDir, "..", "..", "tests", "analysis")
        loc = os.path.abspath(loc)
        if os.path.isdir(loc):
            return loc
        else:
            raise RuntimeError("Expected the analysis tests directory at '%s' but it is not a directory " % loc)

    def __getDataDirsAsString(self):
        return self._dataDirs

    def __moveFile(self, src, dst):
        if os.path.exists(src):
            shutil.move(src, dst)

    def __copyFile(self, src, dst):
        if os.path.exists(src):
            shutil.copyfile(src, dst)

    saveDir = property(lambda self: self.__saveDir)
    testDir = property(lambda self: self.__testDir)
    dataDir = property(lambda self: self.__dataDirs)

    def config(self):
        # backup the existing user properties so we can step all over it
        self.__userPropsFile = config.getUserFilename()
        self.__userPropsFileBackup = self.__userPropsFile + ".bak"
        self.__userPropsFileSystest = self.__userPropsFile + ".systest"
        self.__moveFile(self.__userPropsFile, self.__userPropsFileBackup)

        # Make sure we only save these keys here
        config.reset()

        # Up the log level so that failures can give useful information
        config['logging.loggers.root.level'] = self.__loglevel
        # Set the correct search path
        config['datasearch.directories'] = self.__dataDirs

        # Save path
        config['defaultsave.directory'] = self.__saveDir

        # Do not show paraview dialog
        config['paraview.ignore'] = "1"

        # Do not update instrument definitions
        config['UpdateInstrumentDefinitions.OnStartup'] = "0"

        # Do not perform a version check
        config['CheckMantidVersion.OnStartup'] = "0"

        # Disable usage reports
        config['usagereports.enabled'] = "0"

        # Case insensitive
        config['filefinder.casesensitive'] = 'Off'

        # Maximum number of threads
        config['MultiThreaded.MaxCores'] = '2'

        # datasearch
        if self.__datasearch:
            # turn on for 'all' facilities, 'on' is only for default facility
            config["datasearch.searcharchive"] = 'all'
            config['network.default.timeout'] = '5'

        # Save this configuration
        config.saveConfig(self.__userPropsFile)

    def restoreconfig(self):
        self.__moveFile(self.__userPropsFile, self.__userPropsFileSystest)
        self.__moveFile(self.__userPropsFileBackup, self.__userPropsFile)


#########################################################################
# Function to return a string describing the environment
# (platform) of this test.
#########################################################################
def envAsString():
    """Returns a string describing the environment
    (platform) of this test."""
    if os.name == 'nt':
        system = platform.system().lower()[:3]
        arch = platform.architecture()[0][:2]
        env = system + arch
    elif os.name == 'mac':
        env = platform.mac_ver()[0]
    else:
        env = platform.dist()[0] + "-" + platform.dist()[1]
    return env


#########################################################################
# Function to keep a pool of threads active in a loop to run the tests.
# Each thread starts a loop and gathers a first test module from the
# master test list which is stored in the tests_dict shared dictionary,
# starting with the number in the module list equal to the process id.
#
# Each process then checks if all the data files required by the current
# test module are available (i.e. have not been locked by another
# thread). If all files are unlocked, the thread proceeds with that test
# module. If not, it goes further down the list until it finds a module
# whose files are all available.
#
# Once it has completed the work in the current module, it checks if the
# number of modules that remains to be executed is greater than 0. If
# there is some work left to do, the thread finds the next module that
# still has not been executed (searches through the tests_lock array
# and finds the next element that has a 0 value). This aims to have all
# threads end calculation approximately at the same time.
#########################################################################
def testThreadsLoop(testDir, saveDir, dataDir, options, tests_dict,
                    tests_lock, tests_left, res_array, stat_dict,
                    total_number_of_tests, maximum_name_length,
                    tests_done, process_number, lock, required_files_dict,
                    locked_files_dict):

    reporter = XmlResultReporter(showSkipped=options.showskipped,
                                 total_number_of_tests=total_number_of_tests,
                                 maximum_name_length=maximum_name_length)

    runner = TestRunner(executable=options.executable, exec_args=options.execargs,
                        escape_quotes=True, clean=options.clean)

    # Make sure the status is 1 to begin with as it will be replaced
    res_array[process_number + 2*options.ncores] = 1

    # Begin loop: as long as there are still some test modules that
    # have not been run, keep looping
    while (tests_left.value > 0):
        # Empty test list
        local_test_list = None
        # Get the lock to inspect the global list of tests
        lock.acquire()
        # Run through the list of test modules, starting from the ith
        # element where i is the process number.
        for i in range(process_number,len(tests_lock)):
            # If the lock for this particular module is 0, it means
            # this module has not yet been run and it will be chosen
            # for this particular loop
            if tests_lock[i] == 0:
                # Check for the lock status of the required files for this test module
                modname = tests_dict[str(i)][0]._modname
                no_files_are_locked = True
                for f in required_files_dict[tests_dict[str(i)][0]._modname]:
                    if locked_files_dict[f]:
                        no_files_are_locked = False
                        break
                # If all failes are available, we can proceed with this module
                if no_files_are_locked:
                    # Lock the data files for this test module
                    for f in required_files_dict[modname]:
                        locked_files_dict[f] = True
                    # Set the current test list to the chosen module
                    local_test_list = tests_dict[str(i)]
                    tests_lock[i] = 1
                    imodule = i
                    tests_left.value -= 1
                    break
        # Release the lock
        lock.release()

        # Check if local_test_list exists: if all data was locked,
        # then there is no test list
        if local_test_list:

            if (not options.quiet):
                print("##### Thread %2i will execute module: [%3i] %s (%i tests)" \
                       % (process_number, imodule, modname, len(local_test_list)))
                sys.stdout.flush()

            # Create a TestManager, giving it a pre-compiled list_of_tests
            mgr = TestManager(test_loc=testDir,
                              runner=runner,
                              output=[reporter],
                              quiet=options.quiet,
                              testsInclude=options.testsInclude,
                              testsExclude=options.testsExclude,
                              exclude_in_pr_builds=options.exclude_in_pr_builds,
                              showSkipped=options.showskipped,
                              output_on_failure=options.output_on_failure,
                              process_number=process_number,
                              ncores=options.ncores,
                              clean=options.clean,
                              list_of_tests=local_test_list)

            try:
                mgr.executeTests(tests_done)
            except KeyboardInterrupt:
                mgr.markSkipped("KeyboardInterrupt", tests_done.value)

            # Update the test results in the array shared across cores
            res_array[process_number] += mgr._skippedTests
            res_array[process_number + options.ncores] += mgr._failedTests
            res_array[process_number + 2*options.ncores] = min(int(reporter.reportStatus()),\
                res_array[process_number + 2*options.ncores])

            # Delete the TestManager
            del mgr

            # Unlock the data files
            lock.acquire()
            for f in required_files_dict[modname]:
                locked_files_dict[f] = False
            lock.release()

    # Report the errors
    local_dict = dict()
    with open(os.path.join(saveDir, "TEST-systemtests-%i.xml" % process_number),
              'w') as xml_report:
        xml_report.write(reporter.getResults(local_dict))

    for key in local_dict.keys():
        stat_dict[key] = local_dict[key]

    return
