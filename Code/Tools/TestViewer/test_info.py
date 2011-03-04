# -*- coding: utf-8 -*-
""" Classes describing test projects,
how they are run,
and interpreting the results"""
import time
import datetime
import os
import commands
import tempfile
import shutil
from xml.dom.minidom import parse, parseString
import multiprocessing
from multiprocessing import Pool
import random
import subprocess
import sys
import shlex
import threading

#==================================================================================================
# GLOBAL CONSTANTS
global MSG_ALL_BUILDS_SUCCESSFUL
MSG_ALL_BUILDS_SUCCESSFUL = "MSG(all_builds_successful)"

html_escape_table = {
    #"&": "&amp;",
    #'"': "&quot;",
    #"'": "&apos;",
    #">": "&gt;",
    "<": "&lt;",
    #'‘': "&apos;",
    }

def html_escape(text):
    """Produce entities within text."""
    out = text
    for (key, val) in html_escape_table.items():
        out = out.replace(key, val)
    return out
    #return "".join(html_escape_table.get(c,c) for c in text)



#==================================================================================================
# GLOBAL VARIABLES

# Limit in memory to a process (single test suite)
memory_limit_kb = 800000

# Limit of time that a suite can run
process_timeout_sec = 1

#==================================================================================================
class TestResult:
    """Enumeration giving the state of a single test, suite, or project"""
    """ Test was not run since the program started """
    NOT_RUN = 0
    """ Test passed """
    ALL_PASSED = 1
    """ At least one test failed """
    SOME_FAILED = 2
    """ Build error ! """
    BUILD_ERROR = 3
    """ All tests failed """
    ALL_FAILED = 4
    """ Probably a segfault """ 
    ABORTED = 5
    
    def __init__(self, value=0, old=False):
        self.value = value
        self.old = old
        
    def is_failed(self):
        return self.value == self.SOME_FAILED or self.value == self.BUILD_ERROR \
               or self.value == self.ALL_FAILED or self.value == self.ABORTED
        
    def __eq__(self, other):
        """ Equality comparison """
        if isinstance(other, TestResult):
            return ((self.value == other.value) and (self.old == other.old))
        else:            
            return self.value == other
        
    def __neq__(self, other):
        if isinstance(other, TestResult):
            return (self.value != other.value) or (self.old != other.old)
        else:
            return self.value != other
        
    def get_string(self):
        """Return a string summarizing the state. Used in GUI."""
        s = "Unknown"
        if self.value == self.NOT_RUN: s = "Not Run" 
        if self.value == self.ALL_PASSED: s = "All Passed" 
        if self.value == self.SOME_FAILED: s = "FAILED!" 
        if self.value == self.BUILD_ERROR: s = "BUILD ERROR!" 
        if self.value == self.ALL_FAILED: s = "ALL FAILED!"
        if self.value == self.ABORTED: s = "ABORTED!"
        if self.old and (self.value != self.NOT_RUN):
            s += " (old)"
        return s 
        
        
    def add(self, other):
        """ Add the state from a another test result or another suite. """
        if other == self.BUILD_ERROR:
            self.value = self.BUILD_ERROR
            return
        
        if other == self.ALL_PASSED:
            if self.value == self.ALL_FAILED:
                self.value = self.SOME_FAILED
            elif self.value == self.NOT_RUN:
                self.value = self.ALL_PASSED
        
        if other == self.ALL_FAILED or other == self.ABORTED:
            if self.value == self.ALL_PASSED:
                self.value = self.SOME_FAILED
            elif self.value == self.NOT_RUN:
                self.value = self.ALL_FAILED
        
        if other == self.SOME_FAILED:
            if (self.value == self.ALL_PASSED) or (self.value == self.ALL_FAILED):
                self.value = self.SOME_FAILED
            elif self.value == self.NOT_RUN:
                self.value = self.SOME_FAILED
                
        # If anything is old, then this one is old too!
        if isinstance(other, TestResult):
            if other.old and other.value != self.NOT_RUN:
                self.old = True
        
     
          
#==================================================================================================
class TestSingle(object):
    """ A single test instance (one test inside one suite) """
    def __init__(self, name, parent, fullname=None):
        self.name = name
        if fullname is None:
            self.fullname = name
        else:
            self.fullname = fullname
        
        # Parent TestSuite object
        self.parent = parent
        
        # Starting test state
        self.state = TestResult()
        
        # Last date and time the test was run
        self.lastrun = None
        
        # Time (in seconds) to execute the test
        self.runtime = 0.0
        
        # Last failure text
        self.failure = ""
        # Line in the file of the failure
        self.failure_line = 0
        
        # Stdout output for that failure 
        self.stdout = "" 
        
    #----------------------------------------------------------------------------------
    def get_failed(self):
        """Return 1 if the test failed"""
        if self.state.is_failed():
            return 1
        else:
            return 0
    failed = property(get_failed) 
    
    #----------------------------------------------------------------------------------
    def get_fullname(self):
        """Return a full, uniquely identifying name for this """
        return self.fullname
    
    #----------------------------------------------------------------------------------
    def get_results_text(self):
        """Returns HTML text describing these test results """
        # Change the header color
        color = ['"green"', '"red"'][self.failed]
        s = u"<font color=%s><b>%s</b></font><br />" % (color, self.name)
#        if len(self.failure) > 0:
#            s += self.failure + "<br>"
        if len(self.stdout) > 0:
            s += u'<pre style="white-space: pre-wrap;">'
            s +=  unicode(self.stdout)
            #s += html_escape( unicode(self.stdout) )
            s += u"</pre>"
#            lines = self.stdout.split("\n")
#            # Remove any empty first line
#            if lines[0] == "" and len(lines)>1: lines = lines[1:]
#            # Use the pre tag but wrap long lines.
#            s += u'<pre style="white-space: pre-wrap;">'
#            # Print the rest
#            for line in lines: 
#                s += unicode( html_escape(line) + "\n")
#            s += u"</pre>"
        return s
        
    #----------------------------------------------------------------------------------
    def replace_contents(self, other):
        """ Replace the contents of self with those of other (coming after running in
        a separate thread """
        self.name = other.name
        self.state = other.state
        self.lastrun = other.lastrun
        self.runtime = other.runtime
        self.failure = other.failure
        self.failure_line = other.failure_line
        self.stdout = other.stdout
        
    #----------------------------------------------------------------------------------
    def load_results(self, case):
        """Load the results from a xml Junit file 
        Parameters
            case : a xml.Node object containing the testcase xml results """
        # Get the runtime
        self.runtime = float(case.getAttribute("time"))
        # Assumed passed
        self.state = TestResult(TestResult.ALL_PASSED, old=False)
        # Look for failures
        fails = case.getElementsByTagName("failure")
        
        if len(fails)>0:
            self.state = TestResult(TestResult.ALL_FAILED, old=False)
            # File and line of failure
            file = fails[0].getAttribute("file")
            self.failure_line = fails[0].getAttribute("line")
            # Get the failure text
            self.failure = fails[0].firstChild.data

        # Get the system output
        systemout = case.getElementsByTagName("system-out")
        if len(systemout) > 0:
            # This is a node containing text (the firstchild) which is a Text node
            self.stdout = systemout[0].firstChild.data
        else:
            self.stdout = ""
            
    #----------------------------------------------------------------------------------
    def get_state_str(self):
        """Return a string summarizing the state. Used in GUI."""
        return self.state.get_string()
        
    def age(self):
        """ Age the results (flag them as "old" ) """
        self.state.old = True
        
    def __repr__(self):
        return "TestSingle(%s): state=%s, lastrun=%s, runtime=%s.\n%s" % (self.name, self.get_state_str(), self.lastrun, self.runtime, self.stdout)
        

#==================================================================================================
class TestSuite(object):
    """ A suite of tests """

    def __init__(self, name, parent, classname, command, rundir, xml_file, source_file, log_file):
        """ Constructor"""
        # Its own name, e.g. "UnitTest"
        self.name = name
        # Name of the parent project, e.g. "KernelTest"
        self.parent = parent
        # Full class name, e.g. KernelTest.UnitTest
        self.classname = classname
        # Full command that runs the test suite
        self.command = command
        # Name of the XML file produced when running (no path)
        self.xml_file = xml_file
        # Name of the mantid.log file produced for this test
        self.log_file = log_file
        # Run directory
        self.rundir = rundir
        # Marker for when the contained suites changed (some were added or removed)
        self.contents_changed = False
        # Last date and time the test was run
        self.lastrun = None
        
        # Source file (BlaBlaTest.h) for this suite
        self.source_file = source_file
        if not os.path.exists(self.source_file):
            print "Warning! Source file for test %s not found: %s" % (self.name, self.source_file)
            self.source_file_mtime = time.time()
        else:
            # Last modified time of the source file
            self.source_file_mtime = os.path.getmtime(self.source_file)
        
        # A list of test singles inside this suite
        self.tests = []
        # Is it selected to run?
        self.selected = True
        # Was it made correctly?
        self.build_succeeded = True
        self.build_stdout = ""
        # The state of the overall suite
        self.state = TestResult()
        self.passed = 0
        self.failed = 0
        self.num_run = 0
        # Contents of the mantid.log file
        self.log_contents = ""

    #----------------------------------------------------------------------------------
    def get_fullname(self):
        """Return a full, uniquely identifying name for this """
        return self.classname
            
    #----------------------------------------------------------------------------------
    def get_results_text(self, details=True):
        """Returns HTML text describing these test results """
        # Change the header color
        color = ['"green"', '"red"'][self.failed > 0]
        s = u"<font color=%s><h3>%s</h3></font>" % (color, self.name + ": " + self.get_state_str())
        if not self.build_succeeded:
            s += u'<pre style="white-space: pre-wrap;">'
            s += unicode( html_escape( self.build_stdout) )
            s += u"</pre>"
        else:
            for test in self.tests:
                if details or test.failed:
                    s += test.get_results_text()
                    
            # Now the mantid.log, if any
            if details:
                if self.log_contents == "":
                    s += "<br>No log file found.</br>"
                else:
                    s += u"<font color=Orange><h3>%s</h3></font>" % (self.name + ": " + "Log File")
                    s += "<br>"
                    html_log = html_escape( self.log_contents )
                    html_log = html_log.replace("\n", "<br>")
                    s += html_log
        return s

    #----------------------------------------------------------------------------------
    def replace_contents(self, other):
        """ Replace the contents of self with those of other (coming after running in
        a separate thread """
        if len(self.tests) != len(other.tests):
            print "The number of tests in %s changed." % self.classname
            # We replace the entire list
            self.tests = other.tests
            # And this tells the tree view that it needs to update itself entirely.
            self.contents_changed = True
        else:
            for i in xrange(len(self.tests)):
                if (self.tests[i].name != other.tests[i].name):
                    self.contents_changed = True
                self.tests[i].replace_contents( other.tests[i] )
        # Copy local values
        self.lastrun = other.lastrun
        self.build_succeeded = other.build_succeeded
        self.build_stdout = other.build_stdout
        self.log_contents = other.log_contents
        # Re-compile the states from the individual tests
        self.compile_states()
        
        
    #----------------------------------------------------------------------------------
    def add_single(self, test_name, fullname):
        """ Add a single test to this suite """
        self.tests.append( TestSingle(test_name, self, fullname) )
        
    #----------------------------------------------------------------------------------
    def get_parent(self):
        """ Return the parent Project of this suite """
        return self.parent
    
    #----------------------------------------------------------------------------------
    def get_modified(self):
        """" Returns True if the required source file was modified. 
        NOTE: This overwrites the previous cached modified time, AKA it will
        only return True once per change."""
        oldtime = self.source_file_mtime
        if os.path.exists(self.source_file):
            self.source_file_mtime = os.path.getmtime(self.source_file)
        return (self.source_file_mtime != oldtime)
        
    def get_selected(self):
        return self.selected
    
    #----------------------------------------------------------------------------------
    def set_selected(self, value):
        """Sets the selection state of this suite. """
        self.selected = value
        #if self.parent.selected:
        
    #----------------------------------------------------------------------------------
    def age(self):
        """ Age the results (flag them as "old" ) """
        self.state.old = True
        for test in self.tests:
            test.age()

    #----------------------------------------------------------------------------------
    def is_built(self):
        """Returns True if the test build for this suite was successful."""
        return self.build_succeeded
    
    #----------------------------------------------------------------------------------
    def set_build_failed(self, output):
        """Sets that the build failed for all single tests in this suite.
        Parameters:
            output: stdout from the make command
        """
        self.build_succeeded = False
        self.build_stdout = output
        for test in self.tests:
            test.state = TestResult(TestResult.BUILD_ERROR, old=False)
            test.failure = "Build failure"
            test.stdout = ""
            
    #----------------------------------------------------------------------------------
    def compile_states(self):
        """ Add up the single test results into this suite """
        self.state = TestResult(TestResult.NOT_RUN)
        self.passed = 0
        self.failed = 0
        self.num_run = 0
        for test in self.tests:
            self.state.add( test.state )
            if test.state.is_failed():
                self.failed += 1 
                self.num_run += 1
            else:
                self.passed += 1 
                self.num_run += 1
            
    #----------------------------------------------------------------------------------
    def get_runtime(self):
        """Return the total runtime of contained tests """
        runtime = 0
        for test in self.tests:
            runtime += test.runtime
        return runtime
    runtime = property(get_runtime) 
    
        
    #----------------------------------------------------------------------------------
    def get_state_str(self):
        """Return a string summarizing the state. Used in GUI."""
        self.compile_states()
        if self.failed > 0:
            return self.state.get_string() + " (%d of %d failed)" % (self.failed, self.num_run) #, self.num_run)
        else:
            return self.state.get_string() + " (%d)" % (self.num_run) #, self.num_run)
       
        
    #----------------------------------------------------------------------------------
    def run_tests(self, stdout_callback_func):
        """ Runs this test suite, then loads the produced XML file
        and interprets its results.
        This method should be written so that it can be run in parallel. """
        
        self.contents_changed = False
        
        # Present working directory
        pwd = os.getcwd()
        
        make_temp_dir = False
        if make_temp_dir:
            # Create a temporary directory just for running this test suite
            tempdir = tempfile.mkdtemp()
            rundir = tempdir
        else:
            rundir = self.rundir
            
        os.chdir(rundir)
        
        # In order to catch "segmentation fault" message, we call bash and get the output of that!
        # Max memory for process in KB is a global
        full_command = "bash -c 'ulimit -v %d ; %s'" % (memory_limit_kb, self.command)
        
        
        # Execute the test command; wait for it to return, up to a timeout
        (status, output) = run_command_with_timeout(full_command, process_timeout_sec, run_shell=False)
        
        if status == -15:
            output = output + "\n\nPROCESS TIMED OUT"
            
        # Get the output XML filename
        xml_path = os.path.join(rundir, self.xml_file)
        if os.path.exists(xml_path) and os.path.getsize(xml_path) > 0:
            # Yes, something was output
            self.parse_xml(xml_path) 
        else:
            # No - you must have segfaulted or some other error!
            self.set_aborted(output)
            
        # Now try to load the log_file
        if os.path.exists(self.log_file):
            self.log_contents = open(self.log_file).read()
        else:
            self.log_contents = ""
            
        # Go back to old directory and remove the temp one
        os.chdir(pwd)
        
        if make_temp_dir:
            try:
                shutil.rmtree(tempdir)
            except:
                print "Error removing temporary directory ", tempdir
            
        # Finalize
        self.compile_states()  
        
    #----------------------------------------------------------------------------------
    def set_aborted(self, stdout):
        """ Set that all tests aborted and save the stdout """
        for test in self.tests:
            test.state.value = self.state.ABORTED
            test.state.old = False
            test.stdout = stdout
        
        
    #----------------------------------------------------------------------------------
    def find_test(self, test_name):
        """Find and return a TestSingle instance of given name"""
        for test in self.tests:
            if test.name == test_name:
                return test
        return None
        

    #----------------------------------------------------------------------------------
    def parse_xml(self, xml_path):
        """Interpret a jUnit-style XML file produced for this suite.
        
        Parameters
            xml_path :: full path to the produced XML path"""
            
        self.lastrun = datetime.datetime.now()
           
        try:
            dom = parse(xml_path)
        except:
            # Empty file, for example? Just return
            return
        
        #print dom.getElementsByTagName(self.name)
        suites = dom.getElementsByTagName("testsuite")
        if len(suites) == 0:
            return
        elif len(suites) > 1:
            for xmlSuite in suites:
                if (suites[0].getAttribute("name") == self.name):
                    break
        else:
            xmlSuite = suites[0]
        
        # Get all the test cases (aka TestSuite)
        xmlCases = xmlSuite.getElementsByTagName("testcase")
        for case in xmlCases:
            classname = case.getAttribute("classname")
            if (classname == self.classname):
                # This is the single test name
                test_name = case.getAttribute("name")
                test = self.find_test(test_name)
                # It is possible that the test was just added
                if test is None:
                    #print "Test %s in suite %s was not found. Adding it." % (test_name, classname)
                    self.add_single(test_name, self.classname+"."+test_name)
                    # Look for it now
                    test = self.find_test(test_name)
                    # Mark that we need to update the tree in the GUI
                    self.contents_changed = True

                if not test is None:
                    # Save the time
                    test.lastrun = self.lastrun
                    test.load_results(case)
                else:
                    print "Was unable to parse results of test %s.%s!" % (classname, test_name)
                
                
        # Now we look for tests that are no longer in the suite, and remove them
        tests_copy = self.tests[:]
        for test in tests_copy:
            if test.lastrun != self.lastrun:
                #print "Removing test %s" % test.get_fullname()
                self.tests.remove(test)
                # Mark that we need to update the tree in the GUI
                self.contents_changed = True
            
    #----------------------------------------------------------------------------------
    def __repr__(self):
        return "TestSuite(%s) with %d TestSingle(s).\nCommand=%s\nXML File=%s\nSource file=%s" % (self.name, len(self.tests), self.command, self.xml_file, self.source_file)
        


#==================================================================================================
class TestProject(object):
    """ A sub-project of several test suites, e.g. KernelTest """
    
    #----------------------------------------------------------------------------------
    def __init__(self, name, executable, make_command):
        self.name = name
        
        # Path to the executable command
        self.executable = executable
        
        # Command that will build the given executable
        self.make_command = make_command
        
        # Test suites in this project
        self.suites = []
        
        # Is it selected to run?
        self.selected = True

        # The state of the overall project
        self.state = TestResult()
        self.passed = 0
        self.failed = 0
        self.num_run = 0
        self.build_succeeded = True
        self.build_stdout = ""

    #----------------------------------------------------------------------------------
    def get_lastrun(self):
        """Return the last time any of the suites were run""" 
        latest = datetime.datetime(2000,1,1)
        for suite in self.suites:
            if not suite.lastrun is None:
                if suite.lastrun > latest:
                    latest = suite.lastrun
        if latest != datetime.datetime(2000,1,1):
            return latest
        else:
            return None
    lastrun = property(get_lastrun) 

    #----------------------------------------------------------------------------------
    def get_fullname(self):
        """Return a full, uniquely identifying name for this """
        return self.name
    
    #----------------------------------------------------------------------------------
    def get_selected(self):
        """Return whether this is selected or not. NOTE:
        Returns: 0: none are selected; 1: all are selected; 2: some are selected"""
        num_sel = 0
        num_not_sel = 0
        for suite in self.suites:
            if suite.selected: 
                num_sel += 1
            else:
                num_not_sel += 1
                
        if num_sel > 0 and num_not_sel == 0:
            return 1
        if num_not_sel > 0 and num_sel == 0:
            return 0
        return 2
            
    #----------------------------------------------------------------------------------
    def get_results_text(self):
        """Returns HTML text describing these test results """
        # Change the header color
        color = ['"green"', '"red"'][self.failed > 0]
        s = u"<font color=%s><h1>%s</h1></font>" % (color, self.name + ": " + self.get_state_str())
        if not self.build_succeeded:
            s += u'<pre style="white-space: pre-wrap;">'
            s += unicode( html_escape( self.build_stdout ) )
            s += u"</pre>"
        else:
            for suite in self.suites:
                s += suite.get_results_text(details=False)
        return s
    
    #----------------------------------------------------------------------------------
    def is_source_modified(self, selected_only):
        """Return true if any of the source files were modified 
        @param selected_only :: True if you only check the selected ones."""
        anymod = False
        for suite in self.suites:
            this_one_changed = suite.get_modified()
            if not selected_only or suite.get_selected():
                anymod = anymod or this_one_changed
        return anymod
    
    #----------------------------------------------------------------------------------
    def replace_contents(self, other):
        """ Replace the contents of self with those of other (coming after running in
        a separate thread """
        if len(self.suites) != len(other.suites):
            print "The number of suites in %s changed. You should refresh your view." % self.name
            #TODO! handle better
            self.suites = other.suites
        else:
            for i in xrange(len(self.suites)):
                self.suites[i].replace_contents( other.suites[i] )
        # Re do the stats and states
        self.compile_states()

    #----------------------------------------------------------------------------------
    def make(self, callback_func=None):
        """Make the project using the saved command.
        @param callback_func :: Callback function that will accept a 
               string for each line of the make command's output. """
               
        msg = "-------- Making Test %s ---------" % self.name
        if not callback_func is None: callback_func("%s" % msg)
        
        full_command = self.make_command
        if not callback_func is None: callback_func(full_command)
        # This will run while calling the stdout callback.
        (status, output) = run_command_with_callback(full_command, callback_func)

        if (status != 0):
            msg = "-------- BUILD FAILED! ---------" 
            if not callback_func is None: callback_func("%s" % msg)
            self.build_succeeded = False
            self.build_stdout = output
        else:
            msg = "-------- Build Succeeded ---------" 
            if not callback_func is None: callback_func("%s" % msg)
            self.build_succeeded = True
            # Build was successful
            for suite in self.suites:
                suite.build_succeeded = True
        
        
        
    #----------------------------------------------------------------------------------
    def age(self):
        """ Age the results (flag them as "old" ) """
        self.state.old = True
        for suite in self.suites:
            suite.age()
            
    #----------------------------------------------------------------------------------
    def find_source_file(self, suite_name):
        """ Find the source file corresponding to the given suite in this project
        Returns: the full path to the test file.
        """
        return os.path.join( self.source_path, "test/" + suite_name + ".h")
    
    
    #----------------------------------------------------------------------------------
    def is_anything_selected(self):
        """Return True if any of the suites are selected."""
        for suite in self.suites:
            if suite.selected:
                return True
        return False
                    
    #----------------------------------------------------------------------------------
    def get_runtime(self):
        """Return the total runtime of contained tests """
        runtime = 0
        for suite in self.suites:
            runtime += suite.get_runtime()
        return runtime
    runtime = property(get_runtime) 
                       
    #----------------------------------------------------------------------------------
    def compile_states(self):
        """ Add up the single test results into this suite """
        self.state = TestResult(TestResult.NOT_RUN)
        self.passed = 0
        self.failed = 0
        self.num_run = 0
        for suite in self.suites:
            state = suite.state 
            self.state.add( state )
            self.passed += suite.passed 
            self.num_run += suite.num_run
            self.failed += suite.failed
                    
    #----------------------------------------------------------------------------------
    def get_state_str(self):
        """Return a string summarizing the state. Used in GUI."""
        self.compile_states()
        if self.failed > 0:
            return self.state.get_string() + " (%d of %d failed)" % (self.failed, self.num_run) #, self.num_run)
        else:
            return self.state.get_string() + " (%d)" % (self.num_run) #, self.num_run)
       
    #----------------------------------------------------------------------------------
    def populate(self, project_source_path):
        """ Discover the suites and single tests in this test project. 
        @param project_source_path :: root path to the project. e.g. Framework/Kernel
        """
        self.suites = []
        self.source_path = project_source_path
        
        # CXX test simply lists "TestSuite testName"
        last_suite_name = ""
        suite = None
        
        # Get the bare XML file name
        (dir, file) = os.path.split(self.executable)
        
        output = commands.getoutput(self.executable + " --help-tests")
        xml_file = "TEST-%s.xml" % self.name
        # The silly cxxtest makes an empty XML file
        try:
            os.remove(xml_file)
        except:
            pass
                
        lines =  output.split("\n")
        # Look for the The first two lines are headers
        count = 0
        while count < len(lines) and not lines[count].startswith("------------------------------------------------------"):
            count += 1
        count += 1
        if count >= len(lines):
            print "Error interpreting CXX test output of %s" % (self.executable)
        
        lines = lines[count:]
        
        for line in lines:
            words = line.split()
            if len(words) == 2:
                suite_name = words[0]
                test_name = words[1]
                if suite_name != "" and test_name != "":
                    # Are we making a new suite?
                    if last_suite_name != suite_name:

                        # The class name goes KernelTest.DateAndTimeTest
                        classname = self.name + "." + suite_name
                        source_file = self.find_source_file(suite_name)
                        # The xml file output goes (as of rev 8587, new cxxtestgen see ticket #2204 )
                        xml_file = "TEST-" + classname + ".xml"     
                        # The shell command to run   
                        log_file = classname + ".log"
                        command = "MANTIDLOGPATH=%s" % log_file + " " + self.executable + " " + suite_name
                        # Create that suite
                        suite = TestSuite(suite_name, self, classname, 
                                          command,
                                          dir, xml_file, source_file, log_file)
                        last_suite_name = suite_name
                        self.suites.append(suite)
                        
                    # Add a single test to whatever suite we are in
                    suite.add_single(test_name, classname+"."+test_name)
            else:
                # Could not interpret line
                pass
            
    def __repr__(self):
        return "TestProject(%s)" % (self.name)


#==================================================================================================
#======== Global methods used by parallel processing ================
#==================================================================================================


#==================================================================================================
def run_tests_in_suite(multiple_tests, suite, stdout_callback_func ):
    """Run all tests in a given suite. Method called
    by the multiprocessing Pool.apply_async() method.
    
    Parameters:
        multiple_tests :: a MultipleProjects instance calling this method.
        suite :: the suite to run
        stdout_callback_func :: callback function for each line of stdout
    """
    if not multiple_tests is None: 
        if multiple_tests.abort_run: return "Aborted."
    if not suite is None:
        suite.run_tests(stdout_callback_func)
    # Simply return the object back (for use by the callback function)
    return suite
        
#==================================================================================================
def make_test(multiple_tests, project):
    """Make the tests in a given project. Method called
    by the multiprocessing Pool.apply_async() method.
    
    Parameters:
        multiple_tests :: a MultipleProjects instance calling this method.
        project :: the project to make
    Returns:
        the project that was just made. Some values will have been set in it;
        the callback function must replace the old project with this one; because
        the changes happened in the OTHER thread!
    """
    if not multiple_tests is None: 
        if multiple_tests.abort_run: return "Aborted."
    if not project is None:
        project.make()
        return project
    else:
        return None
     
     
     
     
#==================================================================================================
#==================================================================================================
#==================================================================================================
class MultipleProjects(object):
    """ A Class containing a list of all the available test projects.
    This will be made into a single global variable instance. """
    
    #--------------------------------------------------------------------------        
    def __init__(self):
        # The projects contained
        self.projects = []
        # Abort flag
        self.abort_run = False
           
        # The state of the overall project
        self.state = TestResult()
        self.passed = 0
        self.failed = 0
        self.num_run = 0
        
        # Head folder of the source files (Framework normally)
        self.source_path = "."
        
    #--------------------------------------------------------------------------        
    def abort(self):
        """ Set a flag to abort all further calculations. """
        print "... Attempting to abort ..."
        self.abort_run = True
        
    #----------------------------------------------------------------------------------
    def age(self):
        """ Age the results (flag them as "old" ) """
        for pj in self.projects:
            pj.age()
            
    #----------------------------------------------------------------------------------
    def select_all(self, value):
        """ Select all tests """
        for pj in self.projects:
            pj.selected = value
            for suite in pj.suites:
                suite.selected = value
        
    #----------------------------------------------------------------------------------
    def select_failed(self):
        """ Select all failing tests """
        for pj in self.projects:
            pj.selected = (pj.failed > 0)
            for suite in pj.suites:
                suite.selected = (suite.failed > 0)
                
    #----------------------------------------------------------------------------------
    def select_svn(self):
        """ Do a 'svn st' call and interpret the results to find which tests need to be run. """
        # First, de-select it all
        self.select_all(False)
        output = commands.getoutput("svn st %s" % self.source_path )
        lines = output.split('\n')
        for line in lines:
            if line.startswith('M') or line.startswith('A') or line.startswith('D') or line.startswith('R'):
                #Change to file or stuff.
                filename = line[8:].strip()
                foundit = None
                for pj in self.projects:
                    for suite in pj.suites:
                        # If the test file and the source file are the same,
                        if os.path.exists(suite.source_file):  
                            if os.path.samefile( suite.source_file, filename):
                                suite.selected = True
                                pj.selected = True
                                foundit = suite
                                break
                
                if foundit is None:
                    # Ok, not directly a test name. Look for a similar test file
                    # Get the bare filename, no .h or .cpp
                    bare_file = os.path.splitext(os.path.basename(filename))[0]
                    for pj in self.projects:
                        for suite in pj.suites:
                            # The words in the source file are inside the Test source file. Might be good.
                            bare_source = os.path.basename(suite.source_file)
                            if bare_file in bare_source:
                                suite.selected = True
                                pj.selected = True
                                foundit = suite
                                break
                if foundit is None:
                    print "%s: No test found." % (filename)
                else:
                    print "%s: Test found: '%s'" % ( filename, foundit.get_fullname() )
                 
            
    #--------------------------------------------------------------------------        
    def discover_CXX_projects(self, path, source_path):
        """Look for CXXTest projects in the given paths.
        Populates all the test in it."""
        
        path = os.path.abspath(path)
        
        self.source_path = source_path
        self.projects = []

        # How many cores to use to make projects        
        num_threads = multiprocessing.cpu_count()-1
        if num_threads < 1: num_threads = 1
        
        testnames = set()
        
        dirList=os.listdir(path)
        for fname in dirList:
            # Look for executables ending in Test
            if fname.endswith("Test") and os.path.isfile(fname):
                testnames.add(fname)
                
        # Now add the known tests, in case they were deleted
        for x in ["AlgorithmsTest", "DataObjectsTest", "MDAlgorithmsTest", "PythonAPITest", "APITest", 
                       "GeometryTest", "MDDataObjectsTest", "CurveFittingTest", "ICatTest", "MDEventsTest", 
                       "DataHandlingTest", "KernelTest", "NexusTest"]:
            testnames.add(x)
        
        for fname in testnames:
            make_command = "cd %s ; make %s -j%d " % (os.path.join(path, ".."), fname, num_threads)
            pj = TestProject(fname, os.path.join(path, fname), make_command)
            print "... Populating project %s ..." % fname

            project_name = fname.replace("Test", "")
            project_source_path = os.path.join(source_path, project_name)

            pj.populate(project_source_path)
            self.projects.append(pj)
        
    #--------------------------------------------------------------------------        
    def make_fake_results(self):
        """Generate some fake results for quick debugging """
        for pj in self.projects:
            pj.state.value = random.randint(0,4)
            for suite in pj.suites:
                suite.state.value = random.randint(0,4)
                for test in suite.tests:
                    test.state = TestResult(random.randint(0, 4))
                    test.state.old = (random.randint(0,10) > 5)
                    test.runtime = random.random()/1000
                   
    #----------------------------------------------------------------------------------
    def compile_states(self):
        """ Add up the single test results into this suite """
        self.state = TestResult(TestResult.NOT_RUN)
        self.passed = 0
        self.failed = 0
        self.num_run = 0
        for pj in self.projects:
            state = pj.state 
            self.state.add( state )
            self.passed += pj.passed 
            self.num_run += pj.num_run
            self.failed += pj.failed
                    
    #----------------------------------------------------------------------------------
    def get_state_str(self):
        """Return a string summarizing the state. Used in GUI."""
        if self.failed > 0:
            return self.state.get_string() + " (%d of %d failed)" % (self.failed, self.num_run) #, self.num_run)
        else:
            return self.state.get_string() + " (%d)" % (self.num_run) #, self.num_run)

    #----------------------------------------------------------------------------------
    def is_source_modified(self, selected_only):
        """Return true if any of the source files were modified 
        @param selected_only :: True if you only check the selected ones."""
        anymod = False
        for pj in self.projects:
            if not selected_only or pj.is_anything_selected():
                anymod = anymod or pj.is_source_modified(selected_only)
        return anymod

    #--------------------------------------------------------------------------        
    def find_project(self, name):
        """Return the TestProject named name; None if not found"""
        for pj in self.projects:
            if pj.name == name:
                return pj
        return None
    
    #--------------------------------------------------------------------------        
    def find_suite(self, classname):
        """ Return the TestSuite with the given classname (e.g. KernelTest.UnitTest). 
        Returns None if not found"""
        for pj in self.projects:
            for suite in pj.suites:
                if suite.classname == classname:
                    return suite
        return None
        

    #--------------------------------------------------------------------------        
    def replace_project(self, pj):
        """Given a project from another thread, replace the current one with this one.
        Will look for a matching name"""
        for i in xrange(len(self.projects)):
            if self.projects[i].name == pj.name:
                self.projects[i].replace_contents(pj)
                break
            
    #--------------------------------------------------------------------------        
    def replace_suite(self, st):
        """Given a suite from another thread, replace the current one with this one.
        Will look for a matching name"""
        # Note: You have to re-look for the parent here because st may come from
        #    a different thread = points to the object in another thread
        pj = self.find_project(st.get_parent().name)
        if pj is None:
            return
        for i in xrange(len(pj.suites)):
            suite = pj.suites[i]
            if suite.name == st.name:
                pj.suites[i].replace_contents(st)
                break
        
    #--------------------------------------------------------------------------        
    def get_selected_suites(self, selected_only=True):
        """Returns a list of all selected suites. Suites where make failed
        are excluded!
        
        Parameters:
            selected_only :: set to False to return all suites """
        suites = []
        for pj in self.projects:
            for st in pj.suites:
                # print "get_selected_suites: status of ", st.classname, st.is_built()
                if st.is_built() and (st.get_selected() or (not selected_only)):
                    #print "get_selected_suites adding ", st.classname
                    # Suite must be built to be included here!
                    suites.append(st)
        # print "get_selected_suites ", len(suites)
        return suites             
        
    #--------------------------------------------------------------------------        
    def run_tests_computation_steps(self, selected_only=True, make_tests=True):
        """Returns the number of computation steps that will be done with these parameters
        This is used by the GUI to know how to report progress."""
        count = 0
        if make_tests:
            for pj in self.projects:
                if pj.is_anything_selected() or (not selected_only):
                    count += 1
        count += len(self.get_selected_suites(selected_only))
        return count
                                

    #--------------------------------------------------------------------------        
    def run_tests_in_parallel(self, selected_only=True, make_tests=True, parallel=True, 
                              callback_func=None):
        """Run tests in parallel using multiprocessing.
        Parameters:
            selected_only: run only the selected suites.
            make_tests: set to True to make the tests before running them.
            parallel: set to True to do them in parallel, false to do linearly 
            callback_func: function that takes as argument the suite that was just finished
                or the project that was just made.
                This function MUST replace the original object in order to synchronize the 
                threads' data.
        """
        self.abort_run = False
        
        start = time.time()
        
        # Age all the old results
        self.age()
        
        # How many thread in parallel?  one fewer threads than the # of cpus
        num_threads = multiprocessing.cpu_count()-1
        if num_threads < 1: num_threads = 1
                    
        # Now let's run the make test command for each one
        if make_tests:
            # This is a list of all projects that are needed (selected ones only)
            pj_to_build = []
            for pj in self.projects:
                if pj.is_anything_selected() or (not selected_only):
                    pj_to_build.append(pj)
            
            all_builds_successful = True        
            for pj in pj_to_build:
                # Abort when requested
                if self.abort_run: break
                # Run the make command
                pj.make(callback_func)
                # Callback when completed a project
                if not callback_func is None: 
                    callback_func(pj)
                all_builds_successful = all_builds_successful and pj.build_succeeded
                # Set the build failure to all suites that are selected
                if not pj.build_succeeded:
                    for suite in pj.suites:
                        if (not selected_only) or suite.selected:
                            suite.set_build_failed(pj.build_stdout)


            # Send a message that all builds were good!
            if all_builds_successful and (not callback_func is None):
                callback_func(MSG_ALL_BUILDS_SUCCESSFUL)
                
                    
            
                    
        
        # Build a long list of all (selected and successfully built) suites
        suites = self.get_selected_suites(selected_only)
            
        if parallel:
            # Make the pool 
            p = Pool( num_threads )
            
            # Call the method that will run each suite
            results = []
            for suite in suites:
                result = p.apply_async( run_tests_in_suite, (self, suite, None), callback=callback_func)
                results.append( (result, suite) )
            p.close()
            
            # This will block until completed
            p.join()
            
        else:
            for suite in suites:
                result = run_tests_in_suite( self, suite, callback_func)
                if not callback_func is None: callback_func(result)
                
        # Now we compile all the projects' states
        for pj in self.projects:
            pj.compile_states()
        self.compile_states()
        print "... %s tests %sand completed in %f seconds ..." % (["All", "Selected"][selected_only], ["","built "][parallel],  (time.time() - start))

  
                      
# Global variable containing a list of all the available test projects
global all_tests        
all_tests = MultipleProjects()        

#==================================================================================================
def test_run_print_callback(obj):
    """ Simple callback for running tests. This is called into the MainProcess.
    
    Parameters:
        obj :: the object, either a TestSuite or TestProject that was just calculated
    """
    global all_tests
    if isinstance(obj, TestSuite):
        suite = obj
        print "Done running %s" % suite.classname
        all_tests.replace_suite(suite)
        
    elif isinstance(obj, TestProject):
        pj = obj
        # Replace the project in THIS thread!
        all_tests.replace_project( pj )
        print "Made project %s" % pj.name
        
    else:
        pass
#        print "-->" + obj     
#        sys.stdout.flush() 
          
        


#==================================================================================================
#==================================================================================================
def run_command_with_callback(full_command, callback_func, run_shell=True):
    """Run a shell command while outputting each line to a callback function.
    Parameters:
        full_command :: shell command
        callback_func :: command
    Returns:
        status :: status code
        output :: accumulated stdoutput
    """
    output = ""
    
    if not run_shell:
        full_command = shlex.split(full_command)


    p = subprocess.Popen(full_command, shell=run_shell, bufsize=10000,
                         cwd=".",
                         stdin=None, stderr=subprocess.STDOUT,
                         stdout=subprocess.PIPE, close_fds=True,
                         universal_newlines=True)
    (put, get) = (p.stdin, p.stdout)

    line=get.readline()
    while line != "":
        # Replace annoying character
        line = line.replace('‘', '\'')
        line = line.replace('’', '\'')
        #line = line.replace('\xe2', '\'')
        line = unicode(line)
        # Make one long output string
        output += line
        #Remove trailing /n
        if len(line)>1: line = line[:-1]
        #print line
        if not callback_func is None: callback_func( line )
        #Keep reading output.
        line=get.readline()

        # Do we prematurely abort it?                    
        if all_tests.abort_run:
            p.kill()
            if not callback_func is None: callback_func("Aborted by user.")
            break
        
    # The return code or exit status
    p.wait()
    status = p.returncode
    
    return (status, output)



#==================================================================================================
#==================================================================================================
def run_command_with_timeout(full_command, timeout, run_shell=True):
    """Run a shell command with a timeout.
    
    Parameters:
        full_command :: shell command
        callback_func :: command
    Returns:
        status :: status code
        output :: accumulated stdoutput
    """
    
    # Object to run a command in a process with a timeout.
    class Command(object):
        def __init__(self, cmd, run_shell):
            self.cmd = cmd
            self.process = None
            self.run_shell = run_shell
            self.returncode = 0
            self.output = ""
    
        def run(self, timeout):
            def target():
                self.process = subprocess.Popen(self.cmd, shell=self.run_shell, stdin=None, stderr=subprocess.STDOUT,
                                 stdout=subprocess.PIPE, close_fds=True, universal_newlines=True)
                (self.output, stdin) = self.process.communicate()
    
            thread = threading.Thread(target=target)
            thread.start()
    
            thread.join(timeout)
            if thread.is_alive():
                self.process.terminate()
                thread.join()
                
            self.returncode = self.process.returncode
    
    
    if not run_shell:
        full_command = shlex.split(full_command)

    command = Command(full_command, run_shell)
    command.run(timeout)
        
    output = command.output
    # Status is -15 when it times out
    status = command.returncode
    
    return (status, output)






   
#==================================================================================
#=================== Unit Tests ====================        
#==================================================================================        
def test_results_compiling():
    r = TestResult()
    assert r.value == TestResult.NOT_RUN
    r.add(TestResult.ALL_PASSED)
    assert r.value == TestResult.ALL_PASSED
    r.add(TestResult.ALL_PASSED)
    assert r.value == TestResult.ALL_PASSED
    r.add(TestResult.ALL_FAILED)
    assert r.value == TestResult.SOME_FAILED
    r.add(TestResult.ALL_FAILED)
    assert r.value == TestResult.SOME_FAILED
    r.add(TestResult.ALL_PASSED)
    assert r.value == TestResult.SOME_FAILED
    r.add(TestResult.BUILD_ERROR)
    assert r.value == TestResult.BUILD_ERROR
    
    r = TestResult()
    assert r.value == TestResult.NOT_RUN
    r.add(TestResult.ALL_FAILED)
    assert r.value == TestResult.ALL_FAILED
    r.add(TestResult.ALL_FAILED)
    assert r.value == TestResult.ALL_FAILED
    r.add(TestResult.ALL_PASSED)
    assert r.value == TestResult.SOME_FAILED

def test_age():
    a = TestSingle("my_test_test", None)
    assert (a.state == TestResult.NOT_RUN)
    a.age()
    assert (a.state == TestResult.NOT_RUN)
    assert (a.state.old)
    a = TestSingle("my_test_test", None)
    a.state = TestResult(TestResult.ALL_PASSED)
    a.age()
    assert (a.state.old)
    
test_results_compiling()        
test_age()
        

        
        
#==================================================================================================
if __name__ == '__main__':
#    run_command_with_timeout("echo 'Process started'; sleep 2; echo 'Process finished'", timeout=3)
#    run_command_with_timeout("echo 'Process started'; sleep 2; echo 'Process finished'", timeout=1)
    
    all_tests.discover_CXX_projects("/home/8oz/Code/Mantid/Code/Mantid/bin/", "/home/8oz/Code/Mantid/Code/Mantid/Framework/")
    all_tests.select_svn()
    all_tests.select_all(False)
    suite = all_tests.find_suite("MDEventsTest.MDEventTest")
    suite.set_selected(True)
    suite = all_tests.find_suite("KernelTest.ConfigServiceTest")
    suite.set_selected(True)
    all_tests.run_tests_in_parallel(selected_only=True, make_tests=True, 
                          parallel=True, callback_func=test_run_print_callback)

    
#    for pj in all_tests.projects:
#        print pj.name, pj.get_state_str()
#        for suite in pj.suites:
#            print suite.classname, suite.get_state_str()
#        
#    all_tests.run_tests_in_parallel(selected_only=False, make_tests=True, 
#                          parallel=False, callback_func=test_run_print_callback)




#==================================================================================================
def other_test():
    # Make a sample test project
    kt = TestProject("GeometryTest", "/home/8oz/Code/Mantid/Code/Mantid/bin/GeometryTest", "")
    kt.populate()
    #print kt.suites
    print "----"
    kt.suites[16].run_tests()
        
        
        