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

File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code>.
'''

import sys
import os
import types
import re
import time
import platform
import subprocess
import tempfile

#########################################################################
# The base test class.
#########################################################################
class MantidStressTest(object):
    '''Defines a base class for stress tests, providing functions
    that should be overridden by inheriting classes to perform tests.
    '''

    # Define a delimiter when reporting results
    DELIMITER = '|'
    
    # Define a prefix for reporting results
    PREFIX = 'RESULT'

    def __init__(self):
        pass
    
    def runTest(self):
        raise NotImplementedError('"runTest(self)" should be overridden in a derived class')

    def maxIterations(self):
        '''Override this to perform more than 1 iteration of the implemented test.'''
        return 1

    def reportResult(self, name, value):
        '''
        Send a result to be stored as a name,value pair
        '''
        print self.PREFIX + self.DELIMITER + name + self.DELIMITER + str(value) + '\n',
        
    def execute(self):
        '''
        Run the definined number of iterations of this test
        '''
        # Start timer
        start = time.time()
        self.reportResult('Start time', time.strftime("%d %b %Y %H:%M:%S", time.localtime()))
        self.reportResult('No iterations', self.maxIterations())
        countmax = self.maxIterations() + 1
        for i in range(1, countmax):
            istart = time.time()
            self.runTest()
            delta_t = time.time() - istart
            self.reportResult('Loop ' + str(i) + ' time', '%.2f' % delta_t)
        delta_t = float(time.time() - start)
        # Finish
        self.reportResult('Total time', '%.2f' % delta_t)
                         
    def cleanup(self):
        '''
        This funciton is called after a test has completed and can be used to clean up, i.e.
        remove workspaces etc
        '''
        pass
    
#########################################################################
# A class to store the results of a test 
#########################################################################
class TestResult(object):
    '''
    Stores the results of each test so that they can be reported later
    '''
    
    def __init__(self):
        self._results = []
        pass
    
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
class TextResultReporter(object):
    '''
    Report the results of a test using standard out
    '''
    
    def dispatchResults(self, result):
        '''
        Print the results to standard out
        '''
        nstars = 30
        print '*' * nstars
        for t in result.resultLogs():
            print '\t' + str(t[0]).ljust(15) + '->  ', str(t[1])
        print '*' * nstars

#########################################################################
# A base class for a TestRunner
#########################################################################
class PythonTestRunner(object):
    '''
    A base class to serve as a wrapper to actually run the tests in a specific 
    environment, i.e. console, gui
    '''
    def __init__(self, need_escaping = False):
        self._mtdpy_header = ''
        self._test_dir = ''
        # Get the path that this module resides in so that the tests know about it
        self._framework_path = ''
        for p in sys.path:
            if 'Framework' in p:
                self._framework_path =  os.path.abspath(p).replace('\\','/')
        # A string to prefix the code with
        self._code_prefix = ''
        self._using_escape = need_escaping

    def commandString(self, pycode):
        '''
        Return the appropriate command to pass to subprocess.Popen
        '''
        raise NotImplementedError('"commandString(self)" should be overridden in a derived class')

    def setMantidDir(self, mtdheader_dir):
        # Store the path to MantidPythonAPI
        self._mtdpy_header = os.path.abspath(mtdheader_dir + '/MantidHeader.py').replace('\\','/')

    def setTestDir(self, test_dir):
        self._test_dir = os.path.abspath(test_dir).replace('\\','/')

    def createCodePrefix(self):
        if self._using_escape == True:
            esc = '\\'
        else:
            esc = ''

        self._code_prefix = 'import sys;'
        # On POSIX systems the mantidsimple file is placed in $HOME/.mantid, on windows it will be the current directory
        if os.name == 'posix':
            self._code_prefix += 'sys.path.append(' + esc + '"' + os.environ['HOME'] + '/.mantid' + esc + '");'
        else:
            self._code_prefix += 'sys.path.append(\'.\');'
        self._code_prefix += 'sys.path.append(' + esc + '"' + os.path.dirname(self._mtdpy_header) + esc + '");' + \
        'sys.path.append(' + esc + '"' + self._framework_path + esc + '");' + \
        'sys.path.append(' + esc + '"' + self._test_dir + esc + '");'
        
    def getCodePrefix(self):
        '''
        Return a prefix to the code that will be executed
        '''
        return self._code_prefix

    def spawnSubProcess(self, cmd):
        '''
        Spawn a new process and run the given command within it
        '''
        proc = subprocess.Popen(cmd, shell=True, stdout = subprocess.PIPE)
        std_out = proc.communicate()[0]
        return proc.returncode, std_out

    def start(self, pycode):
        '''
        Run the given test code in a new subprocess
        '''
        raise NotImplementedError('"run(self, pycode)" should be overridden in a derived class')
    
#########################################################################
# A runner class to execute the tests on using the command line interface
#########################################################################
class PythonConsoleRunner(PythonTestRunner):
    '''
    This class executes tests within a Mantid environment inside a standalone python
    interpreter
    '''
    
    def __init__(self):
        PythonTestRunner.__init__(self, True)

    def start(self, pycode):
        '''
        Run the code in a new instance of a python interpreter
        '''
        return self.spawnSubProcess('python -c \"' + self.getCodePrefix() + pycode + '\"')

#########################################################################
# A runner class to execute the tests on using the command line interface
#########################################################################
class MantidPlotTestRunner(PythonTestRunner):
    '''
    This class executes tests within the Python scripting environment inside 
    MantidPlot
    '''
    
    def __init__(self, mtdplot_dir):
        PythonTestRunner.__init__(self)
        mtdplot_bin = mtdplot_dir + '/MantidPlot'
        if os.name == 'nt':
            mtdplot_bin += '.exe'
        self._mtdplot_bin = os.path.abspath(mtdplot_bin).replace('\\','/')
        
    def start(self, pycode):
        '''
        Run the code in a new instance of the MantidPlot scripting environment
        '''
        # The code needs wrapping in a temporary file so that it can be passed to MantidPlot,
        # along with the redirection of the scripting output to stdout
        # On Windows, just using the file given back by tempfile doesn't work as the name is mangled to a short version where
        # all characters after a space are replace by ~. So on windows use put the file in the current directory
        if os.name == 'nt':
            loc = '.'
        else:
            loc = ''
        # MG 11/09/2009: I tried the simple tempfile.NamedTemporaryFile() method but this didn't work
        # on Windows so I had to be a little long winded about it
        fd, tmpfilepath = tempfile.mkstemp(suffix = '.py', dir = loc, text=True)
            
        os.write(fd, 'import sys;sys.stdout = sys.__stdout__;' + self.getCodePrefix() + pycode)
        retcode, output = self.spawnSubProcess(self._mtdplot_bin + ' -xq ' + tmpfilepath) 
        # Remove the temporary file
        os.close(fd)
        os.remove(tmpfilepath)
        return retcode, output
                
#########################################################################
# A class to tie together a test and its results
#########################################################################
class TestSuite(object):
    '''
    Tie together a test and its results.
    '''
    def __init__(self, modname, testname):
        self._modname = modname
        self._fullname = modname + '.' + testname

        self._result = TestResult()
        # Add some results that are not linked to the actually test itself
        self._result.addItem(['Test', self._fullname])
        sysinfo = platform.uname()
        self._result.addItem(['OS', sysinfo[0]])
        self._result.addItem(['Hostname', sysinfo[1]])
        self._result.addItem(['Arch', sysinfo[4]])

    def execute(self, runner):
        print time.strftime("%a, %d %b %Y %H:%M:%S", time.localtime()) + ': Executing ' + self._fullname

        # Construct the code to execute in a separate sub process
        pycode = 'import ' + self._modname + ';' + self._fullname + '().execute()'
        # Start the new process
        retcode, output = runner.start(pycode)
        all_lines = output.split('\n')
        for line in all_lines:
            entries = line.split(MantidStressTest.DELIMITER)
            if len(entries) == 3 and entries[0] == MantidStressTest.PREFIX:
                self._result.addItem([entries[1], entries[2]])
            else:
                print line
                
    def reportResults(self, reporter = TextResultReporter):
        reporter.dispatchResults(self._result)

#########################################################################
# The main API class
#########################################################################
class TestManager(object):
    '''A manager class that is responsible for overseeing the testing process. 
    This is the main interaction point for the framework.
    '''

    def __init__(self, test_dir, mtdheader_dir, runner = PythonConsoleRunner(), reporter = TextResultReporter()):
        '''Initialize a class instance'''

        # Check whether a Mantid.properties file resides in the current directory
        if os.path.isfile('Mantid.properties') == False:
            exit('Cannot find "Mantid.properties" file in the current directory. This is required to continue.')
        
        self._runner = runner
        self._reporter = reporter
        self._test_dir = os.path.abspath(test_dir).replace('\\','/')

        # Pass the Mantid path (the location of the MantidHeader.py file) to the runner class
        runner.setMantidDir(mtdheader_dir)
        runner.setTestDir(self._test_dir)
        runner.createCodePrefix();

        # Need to be able to find the definitions
        sys.path.append(self._test_dir)

        # Init mantid
        sys.path.append(os.path.abspath(mtdheader_dir).replace('\\','/'))
        if os.name == 'posix':
            sys.path.append(os.environ['HOME'] + '/.mantid')
        execfile(runner._mtdpy_header)

    def executeAllTests(self):
        # Get the defined tests
        tests = self.loadTestsFromDir(self._test_dir)
        for suite in tests:
            suite.execute(self._runner)
            suite.reportResults(self._reporter)
         
    def loadTestsFromDir(self, test_dir):
        ''' Load all of the tests defined in the given directory'''
        entries = os.listdir(test_dir)
        tests = []
        regex = re.compile('^.*\.py$', re.IGNORECASE)
        for file in entries:
            if regex.match(file) != None:
                tests.extend(self.loadTestsFromModule(file))
        return tests

    def loadTestsFromModule(self, filename):
        '''
        Load test classes from the given module object which has been
        imported with the __import__ statement
        '''
        tests = []
        modname = filename.split('.py')[0]
        module = __import__(modname)
        for name in dir(module):
            obj = getattr(module, name)
            if type(obj) == types.TypeType and issubclass(obj, MantidStressTest):
                tests.append(TestSuite(modname, name))
        return tests
