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
import datetime
import imp
import inspect
import os
import numpy
import platform
import re
import subprocess
import sys
import tempfile
import time
import unittest
from six import PY3

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
        import mantid.api
        mantid.api.FrameworkManager.clear()
        from mantid.kernel import MemoryStats
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
        Override this to return True if the test is too slow or deemed unnecessary to be run with every pull request.
        These tests will be run nightly instead.
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
        from mantid.api import FileFinder

        # simple way is just getFullPath which never uses archive search
        if os.path.exists(FileFinder.getFullPath(filename)):
            return True

        # try full findRuns which will use archive search if it is turned on
        try:
            candidates = FileFinder.findRuns(filename)
            for item in candidates:
                if os.path.exists(item):
                    return True
        except RuntimeError as e:
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

        # initialize mantid so it can get the data directories to look in
        import mantid
        # check that all of the files exist
        for filename in reqFiles:
            if not self.__verifyRequiredFile(filename):
                print("Missing required file: '%s'" % filename)
                foundAll = False

        if not foundAll:
            sys.exit(TestRunner.SKIP_TEST)


    def __verifyMemory(self):
        """ Do we need to skip due to lack of memory? """
        required = self.requiredMemoryMB()
        if required <= 0:
            return

        # Check if memory is available
        from mantid.kernel import MemoryStats
        MB_avail = MemoryStats().availMem()/(1024.)
        if (MB_avail < required):
            print("Insufficient memory available to run test! %g MB available, need %g MB." % (MB_avail,required))
            sys.exit(TestRunner.SKIP_TEST)


    def execute(self):
        '''
        Run the defined number of iterations of this test
        '''
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
        #self.reportResult('time_taken', '%.2f' % delta_t)

    def __prepASCIIFile(self, filename):
        """
        Prepare an ascii file for comparison using difflib.
        """
        handle = open(filename, mode='r')
        stuff = handle.readlines()
        if self.stripWhitespace:
            stuff = [line.strip() for line in stuff]
        handle.close()
        return stuff

    def validateASCII(self):
        """
        Validate ASCII files using difflib.
        """
        from mantid.api import FileFinder
        (measured, expected) = self.validate()
        if not os.path.isabs(measured):
            measured = FileFinder.Instance().getFullPath(measured)
        if not os.path.isabs(expected):
            expected = FileFinder.Instance().getFullPath(expected)

        measured = self.__prepASCIIFile(measured)
        expected = self.__prepASCIIFile(expected)

        # calculate the difference
        import difflib
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
        from mantid.simpleapi import Load
        numRezToCheck=len(valNames)
        mismatchName=None;

        validationResult =True;
        for ik in range(0,numRezToCheck,2): # check All results
            workspace2 = valNames[ik+1]
            if workspace2.endswith('.nxs'):
                Load(Filename=workspace2,OutputWorkspace="RefFile")
                workspace2 = "RefFile"
            else:
                raise RuntimeError("Should supply a NeXus file: %s" % workspace2)
            valPair=(valNames[ik],"RefFile");
            if numRezToCheck>2:
                mismatchName = valNames[ik];

            if not(self.validateWorkspaces(valPair,mismatchName)):
                validationResult = False;
                print('Workspace {0} not equal to its reference file'.format(valNames[ik]));
        #end check All results

        return validationResult;

    def validateWorkspaceToWorkspace(self):
        '''
        Assumes the second item from self.validate() is an existing workspace
        to compare to the supplied workspace.
        '''
        valNames = list(self.validate())
        return self.validateWorkspaces(valNames)

    def validateWorkspaces(self, valNames=None,mismatchName=None):
        '''
        Performs a check that two workspaces are equal using the CompareWorkspaces
        algorithm. Loads one workspace from a nexus file if appropriate.
        Returns true if: the workspaces match
                      OR the validate method has not been overridden.
        Returns false if the workspace do not match. The reason will be in the log.
        '''
        if valNames is None:
            valNames = self.validate()

        from mantid.simpleapi import SaveNexus, AlgorithmManager
        checker = AlgorithmManager.create("CompareWorkspaces")
        checker.setLogging(True)
        checker.setPropertyValue("Workspace1",valNames[0])
        checker.setPropertyValue("Workspace2",valNames[1])
        checker.setPropertyValue("Tolerance", str(self.tolerance))
        if hasattr(self,'tolerance_is_reller') and self.tolerance_is_reller:
           checker.setPropertyValue("ToleranceRelerr", "1")
        for d in self.disableChecking:
            checker.setPropertyValue("Check"+d,"0")
        checker.execute()
        if not checker.getProperty("Result").value:
            print(self.__class__.__name__)
            if mismatchName:
                SaveNexus(InputWorkspace=valNames[0],Filename=self.__class__.__name__+mismatchName+'-mismatch.nxs')
            else:
                SaveNexus(InputWorkspace=valNames[0],Filename=self.__class__.__name__+'-mismatch.nxs')
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
            return True # don't validate
        method = method.lower()
        if "validateworkspacetonexus".endswith(method):
            return self.validateWorkspaceToNeXus()
        elif "validateworkspacetoworkspace".endswith(method):
            return self.validateWorkspaceToWorkspace()
        elif "validateascii".endswith(method):
            return self.validateASCII()
        else:
            raise RuntimeError("invalid validation method '%s'" % self.validateMethod())

    def returnValidationCode(self,code):
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
        import mantid.api
        mantid.api.FrameworkManager.clear()
        # Get the resident memory again and work out how much it's gone up by (in MB)
        from mantid.kernel import MemoryStats
        memorySwallowed = MemoryStats().residentMem()/1024 - self.memory
        # Store the result
        self.reportResult('memory footprint increase', memorySwallowed )
        return retcode

    def succeeded(self):
        """
        Returns true if the test has been run and it succeeded, false otherwise
        """
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
        """
        Check that a value is within +- delta of the expected value
        """
        # Build the error message
        if msg != "": msg += " "
        msg += "Expected %g == %g within +- %g." % (value, expected, delta)

        if (value > expected+delta) or  (value < expected-delta):
            raise Exception(msg)

    def assertLessThan(self, value, expected, msg=""):
        """
        Check that a value is < expected.
        """
        # Build the error message
        if msg != "": msg += " "
        msg += "Expected %g < %g " % (value, expected)

        if (value >= expected):
            raise Exception(msg)

    def assertGreaterThan(self, value, expected, msg=""):
        """
        Check that a value is > expected.
        """
        # Build the error message
        if msg != "": msg += " "
        msg += "Expected %g > %g " % (value, expected)

        if (value <= expected):
            raise Exception(msg)


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

    def __init__(self):
        '''Initialize a class instance, e.g. connect to a database'''
        pass

    def dispatchResults(self, result):
        raise NotImplementedError('"dispatchResults(self, result)" should be overridden in a derived class')

#########################################################################
# A class to report results as formatted text output
#########################################################################
class TextResultReporter(ResultReporter):
    '''
    Report the results of a test using standard out
    '''

    def dispatchResults(self, result):
        '''
        Print the results to standard out
        '''
        nstars = 30
        print('*' * nstars)
        for t in result.resultLogs():
            print('\t' + str(t[0]).ljust(15) + '->  ', str(t[1]))
        print('*' * nstars)

#########################################################################
# A class to report results as junit xml
#########################################################################
from xmlreporter import XmlResultReporter

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


    def __init__(self, executable, exec_args=None, escape_quotes=False):
        self._executable = executable
        self._exec_args = exec_args
        self._test_dir = ''
        self._escape_quotes = escape_quotes

    def getTestDir(self):
        return self._test_dir

    def setTestDir(self, test_dir):
        self._test_dir = os.path.abspath(test_dir).replace('\\','/')

    def spawnSubProcess(self, cmd):
        '''
        Spawn a new process and run the given command within it
        '''
        proc = subprocess.Popen(cmd, shell = True, stdout = subprocess.PIPE,
                                stderr = subprocess.STDOUT, bufsize=-1)
        std_out, _ = proc.communicate()
        return proc.returncode, std_out

    def start(self, script):
        '''
        Run the given test code in a new subprocess
        '''
        exec_call = self._executable
        if self._exec_args:
            exec_call += ' '  + self._exec_args
        # write script to temporary file and execute this file
        tmp_file = tempfile.NamedTemporaryFile(mode='w', delete=False)
        tmp_file.write(script.asString())
        tmp_file.close()
        cmd = exec_call + ' ' + tmp_file.name
        print("Executing test script '%s'" % (cmd))
        results = self.spawnSubProcess(cmd)
        os.remove(tmp_file.name)
        return results

#########################################################################
# Encapsulate the script for runnning a single test
#########################################################################
class TestScript(object):

    def __init__(self, test_dir, module_name, test_cls_name, exclude_in_pr_builds):
        self._test_dir = test_dir
        self._modname = module_name
        self._test_cls_name = test_cls_name
        self._exclude_in_pr_builds = not exclude_in_pr_builds

    def asString(self):
        code = '''import sys
sys.path.append('{0}')
sys.path.append('{1}')
from {2} import {3}
systest = {3}()
if {5}:
    systest.excludeInPullRequests = lambda: False
systest.execute()
exitcode = systest.returnValidationCode({4})
systest.cleanup()
sys.exit(exitcode)'''
        return code.format(TESTING_FRAMEWORK_DIR, self._test_dir, self._modname,
                           self._test_cls_name, TestRunner.VALIDATION_FAIL_CODE, self._exclude_in_pr_builds)

#########################################################################
# A class to tie together a test and its results
#########################################################################
class TestSuite(object):
    '''
    Tie together a test and its results.
    '''
    def __init__(self, test_dir, modname, testname, filename = None):
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
        self._result.status = 'skipped' # the test has been skipped until it has been executed

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
        print(time.strftime("%a, %d %b %Y %H:%M:%S", time.localtime()) + ': Executing ' + self._fqtestname)
        if self._test_cls_name is not None:
          script = TestScript(self._test_dir, self._modname, self._test_cls_name,exclude_in_pr_builds)
          # Start the new process and wait until it finishes
          retcode, output  = runner.start(script)
        else:
          retcode, output = TestRunner.SKIP_TEST, ""
        self._result.date = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        self._result.addItem(['test_date',self._result.date])

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
        print(output)
        self._result.output = output
        all_lines = output.split('\n')
        # Find the test results
        for line in all_lines:
            entries = line.split(MantidStressTest.DELIMITER)
            if len(entries) == 3 and entries[0] == MantidStressTest.PREFIX:
                self._result.addItem([entries[1], entries[2]])

    def setOutputMsg(self, msg=None):
        if msg is not None:
            self._result.output = msg

    def reportResults(self, reporters):
        for r in reporters:
            r.dispatchResults(self._result)

#########################################################################
# The main API class
#########################################################################
class TestManager(object):
    '''A manager class that is responsible for overseeing the testing process.
    This is the main interaction point for the framework.
    '''

    def __init__(self, test_loc, runner, output = [TextResultReporter()],
                 testsInclude=None, testsExclude=None, exclude_in_pr_builds=None):
        '''Initialize a class instance'''

        # Runners and reporters
        self._runner = runner
        self._reporters = output

        # If given option is a directory
        if os.path.isdir(test_loc) == True:
            test_dir = os.path.abspath(test_loc).replace('\\','/')
            sys.path.append(test_dir)
            runner.setTestDir(test_dir)
            self._tests = self.loadTestsFromDir(test_dir)
        else:
            if os.path.exists(test_loc) == False:
                print('Cannot find file ' + test_loc + '.py. Please check the path.')
                exit(2)
            test_dir = os.path.abspath(os.path.dirname(test_loc)).replace('\\','/')
            sys.path.append(test_dir)
            runner.setTestDir(test_dir)
            self._tests = self.loadTestsFromModule(os.path.basename(test_loc))

        if len(self._tests) == 0:
            print('No tests defined in ' + test_dir + '. Please ensure all test classes sub class stresstesting.MantidStressTest.')
            exit(2)

        self._passedTests = 0
        self._skippedTests = 0
        self._failedTests = 0
        self._lastTestRun = 0

        self._testsInclude = testsInclude
        self._testsExclude = testsExclude
        # flag to exclude slow tests from pull requests
        self._exclude_in_pr_builds = exclude_in_pr_builds

    totalTests = property(lambda self: len(self._tests))
    skippedTests = property(lambda self: (self.totalTests - self._passedTests - self._failedTests))
    passedTests = property(lambda self: self._passedTests)
    failedTests = property(lambda self: self._failedTests)

    def __shouldTest(self, suite):
        if self._testsInclude is not None:
            if not self._testsInclude in suite.name:
                suite.markAsSkipped("NotIncludedTest")
                return False
        if self._testsExclude is not None:
            if self._testsExclude in suite.name:
                suite.markAsSkipped("ExcludedTest")
                return False
        return True

    def executeTests(self):
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
            suite.reportResults(self._reporters)
            self._lastTestRun += 1

    def markSkipped(self, reason=None):
        for suite in self._tests[self._lastTestRun:]:
            suite.setOutputMsg(reason)
            suite.reportResults(self._reporters) # just let people know you were skipped

    def loadTestsFromDir(self, test_dir):
        ''' Load all of the tests defined in the given directory'''
        entries = os.listdir(test_dir)
        tests = []
        regex = re.compile('^.*\.py$', re.IGNORECASE)
        for file in entries:
            if regex.match(file) != None:
                tests.extend(self.loadTestsFromModule(os.path.join(test_dir,file)))
        return tests

    def loadTestsFromModule(self, filename):
        '''
        Load test classes from the given module object which has been
        imported with the __import__ statement
        '''
        modname = os.path.basename(filename)
        modname = modname.split('.py')[0]
        path = os.path.dirname(filename)
        pyfile = open(filename, 'r')
        tests = []
        try:
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
                 data_dirs="", save_dir = "",
                 loglevel='information', archivesearch=False):
        self.__sourceDir = self.__locateSourceDir(sourceDir)

        # add location of stress tests
        self.__testDir = self.__locateTestsDir()

        # add location of the analysis tests
        sys.path.insert(0,self.__locateTestsDir())

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
                search_dir = direc.replace('\\','/')
                if not search_dir.endswith('/'):
                    search_dir += '/'
                    data_path += search_dir + ';'
            #endfor
            self.__dataDirs = data_path

        # set the log level
        self.__loglevel = loglevel
        self.__datasearch =  archivesearch

    def __locateSourceDir(self, suggestion):
        if suggestion is None:
            loc = os.path.abspath(__file__)
            suggestion = os.path.split(loc)[0] # get the directory
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
            import shutil
            shutil.move(src, dst)

    def __copyFile(self, src, dst):
        if os.path.exists(src):
            import shutil
            shutil.copyfile(src, dst)

    saveDir = property(lambda self: self.__saveDir)
    testDir = property(lambda self: self.__testDir)

    def config(self):

        # Start mantid
        import mantid
        from mantid.kernel import config

        # backup the existing user properties so we can step all over it
        self.__userPropsFile = config.getUserFilename()
        self.__userPropsFileBackup  = self.__userPropsFile + ".bak"
        self.__userPropsFileSystest = self.__userPropsFile + ".systest"
        self.__moveFile(self.__userPropsFile, self.__userPropsFileBackup)

        # Make sure we only save these keys here
        config.reset()

        # Up the log level so that failures can give useful information
        config['logging.loggers.root.level'] = self.__loglevel
        config['logging.channels.consoleFilterChannel.level'] = self.__loglevel
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

        # datasearch
        if self.__datasearch:
            # turn on for 'all' facilties, 'on' is only for default facility
            config["datasearch.searcharchive"] = 'all'
            config['network.default.timeout'] = '5'

        # Save this configuration
        config.saveConfig(self.__userPropsFile)

    def restoreconfig(self):
        self.__moveFile(self.__userPropsFile, self.__userPropsFileSystest)
        self.__moveFile(self.__userPropsFileBackup, self.__userPropsFile)


#==============================================================================
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
