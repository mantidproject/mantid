"""
    Defines a handler for the Sphinx 'build-finished' event.
    If the builder is doctest then it post-processes the
    output file to produce an XUnit-style XML file that can be
    more easily parse by CI servers such as Jenkins.

    Output file structure
    ~~~~~~~~~~~~~~~~~~~~~

    The following outcomes are possible for a given
    document:
     - all tests pass;
     - all tests fail;
     - and some test pass and some fail.

    Below are examples of the output for each of the above
    outcomes, given a document named 'FooDoc' in a
    directory 'bar'relative to the documentation root.

    - All Passed:
     ============

    Document: algorithms/AllPassed
    ------------------------------
    2 items passed all tests:
       1 tests in Ex 2
       2 tests in default
    3 tests in 2 items.
    3 passed and 0 failed.
    Test passed.

    Doctest summary
    ===============
    3 tests
    0 failures in tests
    0 failures in setup code
    0 failures in cleanup code

    - All Failed:
     ============

    Document: bar/FooDoc
    --------------------
    **********************************************************************
    File "bar/FooDoc.rst", line 127, in Ex2
    Failed example:
        print "Multi-line failed"
        print "test"
    Expected:
        No match
    Got:
        Multi-line failed
        test
    **********************************************************************
    File "bar/FooDoc.rst", line 111, in Ex1
    Failed example:
        print "Single line failed test"
    Expected:
        No match
    Got:
        Single line failed test
    **********************************************************************
    2 items had failures:
       1 of   1 in Ex1
       1 of   1 in Ex2
    2 tests in 2 items.
    0 passed and 2 failed.
    ***Test Failed*** 2 failures.

    Doctest summary
    ===============
    2 tests
    2 failures in tests
    0 failures in setup code
    0 failures in cleanup code

    - Some pass some fail:
      ====================

    Document: bar/FooDoc
    --------------------
    **********************************************************************
    File "bar/FooDoc.rst", line 127, in default
    Failed example:
        print "A failed test"
    Expected:
        Not a success
    Got:
        A failed test
    **********************************************************************
    File "bar/FooDoc.rst", line 143, in Ex1
    Failed example:
        print "Second failed test"
    Expected:
        Not a success again
    Got:
        Second failed test
    1 items passed all tests:
        1 tests in Ex3
    **********************************************************************
    2 items had failures:
       1 of   1 in Ex1
       1 of   2 in default
    4 tests in 3 items.
    2 passed and 2 failed.
    ***Test Failed*** 2 failures.

    Doctest summary
    ===============
    4 tests
    2 failures in tests
    0 failures in setup code
    0 failures in cleanup code
"""
import re
try:
    import lxml.etree as ElementTree
except ImportError:
    import xml.etree.ElementTree as ElementTree

# Name of file produced by doctest target. It is assumed that it is created
# in app.outdir
DOCTEST_OUTPUT = "output.txt"
# Name of output file that the resultant XUnit output is saved
# @todo make this a configuration variable
XUNIT_OUTPUT = "TEST-doctest.xml"
# Error type string
TEST_FAILURE_TYPE = "UsageFailure"
# Package name
PACKAGE_NAME = "docs"

#-------------------------------------------------------------------------------
# Define parts of lines that denote a document
DOCTEST_DOCUMENT_BEGIN = "Document:"
DOCTEST_SUMMARY_TITLE = "Doctest summary"
FAILURE_MARKER = "*"*70

# Regexes
ALLPASS_TEST_NAMES_RE = re.compile(r"^\s+(\d+) tests in (.+)$")
NUMBER_PASSED_RE = re.compile(r"^(\d+) items passed all tests:$")

TEST_FAILED_END_RE = re.compile(r"\*\*\*Test Failed\*\*\* (\d+) failures.")
FAILURE_LOC_RE = re.compile(r'^File "([\w/\.]+)", line (\d+), in (\w+)$')
MIX_FAIL_RE = re.compile(r'^\s+(\d+)\s+of\s+(\d+)\s+in\s+(\w+)$')

#-------------------------------------------------------------------------------
class TestSuiteReport(object):

    def __init__(self, name, cases, package=None):
        if len(cases) == 0:
            raise ValueError("No test cases provided")
        self.name = name
        self.testcases = cases
        self.package = package

    @property
    def ntests(self):
        return len(self.testcases)

    @property
    def nfailed(self):
        def sum_failure(fails, case):
            if case.failed: return fails + 1
            else: return fails
        return reduce(sum_failure, self.testcases, 0)

    @property
    def npassed(self):
        return self.ntests - self.nfailed

#-------------------------------------------------------------------------------
class TestCaseReport(object):

    def __init__(self, classname, name, failure_descr):
        self.classname = classname
        self.name = name
        if failure_descr is not None:
            self.failure_descr = failure_descr
        else:
            self.failure_descr = ""

    @property
    def passed(self):
        return (self.failure_descr == "")

    @property
    def failed(self):
        return not self.passed

#-------------------------------------------------------------------------------
class DocTestOutputParser(object):
    """
    Process a doctest output file and convert it
    to a different format
    """

    def __init__(self, doctest_output, isfile = True):
        """
        Parses the given doctest output

        Args:
          doctest_output (str): String giving either doctest output as plain
                                text or a filename
          isfile (bool): If True then the doctest_output argument is treated
                          as a filename
        """
        if isfile:
            with open(doctest_output,'r') as results:
                self.testsuite = self.__parse(results)
        else:
            self.testsuite = self.__parse(doctest_output.splitlines())

    def as_xunit(self, filename):
        """
        Write out the test results in Xunit-style format
        """
        suite_node = ElementTree.Element("testsuite")
        suite_node.attrib["name"] = self.testsuite.name
        suite_node.attrib["tests"] = str(self.testsuite.ntests)
        suite_node.attrib["failures"] = str(self.testsuite.nfailed)
        if self.testsuite.package:
            suite_node.attrib["package"] = self.testsuite.package

        cases = self.testsuite.testcases
        for testcase in cases:
            case_node = ElementTree.SubElement(suite_node, "testcase")
            case_node.attrib["classname"] = testcase.classname
            case_node.attrib["name"] = testcase.name
            if testcase.failed:
                failure_node = ElementTree.SubElement(case_node, "failure")
                failure_node.attrib["type"] = TEST_FAILURE_TYPE
                failure_node.text = testcase.failure_descr
        # Serialize to file
        tree = ElementTree.ElementTree(suite_node)
        tree.write(filename, encoding="utf-8", xml_declaration=True)

    def __parse(self, results):
        """
        Parse a doctest output file and a TestSuiteReport
        object that describe the results of the
        all tests on a single document

        Arguments:
          results (iterable): Iterable where each element contains
                              a line of the results

        Returns:
          TestSuite: TestSuite object
        """
        in_doc = False
        document_txt = None
        cases = []
        for line in results:
            line = line.rstrip()
            if line.startswith(DOCTEST_DOCUMENT_BEGIN):
                # parse previous results
                if document_txt:
                    cases.extend(self.__parse_document(document_txt))
                document_txt = [line]
                in_doc = True
                continue
            if line.startswith(DOCTEST_SUMMARY_TITLE): # end of tests
                in_doc = False
                cases.extend(self.__parse_document(document_txt))
                document_txt = None
            if in_doc and line != "":
                document_txt.append(line)
        # endfor
        return TestSuiteReport(name="doctests", cases=cases,
                               package=PACKAGE_NAME)

    def __parse_document(self, results):
        """
        Create a list of TestCaseReport object for this document

        Args:
          results (list): List of lines of doctest output
                          for a single document
        Returns:
          list: List of test cases in the document
        """
        fullname = self.__extract_fullname(results[0])
        if not results[1].startswith("-"):
            raise ValueError("Invalid second line of output: '%s'. "\
                             "Expected a title underline."
                             % text[1])
        results = results[2:] # trim off top two lines
        nfailed = self.__find_number_failed(results)
        if nfailed == 0:
            testcases = self.__parse_success(fullname, results)
        else:
            testcases = self.__parse_failures(fullname, results, nfailed)
        return testcases

    def __extract_fullname(self, line):
        """
        Extract the document name from the line of text.

        Args:
          line (str): Line to test for title
        """
        if not line.startswith(DOCTEST_DOCUMENT_BEGIN):
            raise ValueError("First line of output text should be a line "
                             "beginning '%s'" % DOCTEST_DOCUMENT_BEGIN)
        return line.replace(DOCTEST_DOCUMENT_BEGIN, "").strip()

    def __find_number_failed(self, results):
        """
        Returns:
         Number of failures in the document results
        """
        # Last line should be an overall summary
        lastline = results[-1]
        if lastline.startswith("***") or lastline == "Test passed.":
            match = TEST_FAILED_END_RE.match(lastline)
        else:
            raise ValueError("Unexpected format for last line of document "
                             "results. Expected overall summary, "
                             "found '%s'" % lastline)
        if match: # regex matched
            nfailed = int(match.group(1))
        else:
            nfailed = 0
        return nfailed

    def __parse_success(self, fullname, results):
        """
        Parse results for a success case

        Args:
          fullname (str): String containing full name of document
          results (line): List containing lines of doctest output for
                          document
        Returns:
          A list of test cases that were parsed from the results
        """
        match = NUMBER_PASSED_RE.match(results[0])
        if not match:
            raise ValueError("All passed line incorrect: '%s'"
                             % results[0])
        classname = self.__create_classname(fullname)
        nitems = int(match.group(1))
        cases = []
        for line in results[1:1+nitems]:
            match = ALLPASS_TEST_NAMES_RE.match(line)
            if not match:
                raise ValueError("Unexpected information line in "
                                 "all pass case: %s" % line)
            ntests, name = int(match.group(1)), match.group(2)
            for idx in range(ntests):
                cases.append(TestCaseReport(classname, name, failure_descr=None))
        #endfor
        return cases

    def __parse_failures(self, fullname, results, nfailed):
        """
        Parse text for failures cases for a single document

        Args:
          fullname (str): String containing full name of document
          results (line): List containing lines of doctest output for
                          document
          nfailed (int): The number of failures expected in the
                         document
        Returns:
         A list of test cases that were parsed from the results
        """
        classname = self.__create_classname(fullname)

        # Find index marker lines that delineate failures or
        # a line containing 'items passed all tests:'. It looks as if
        # this is a bug in sphinx.doctest output that doesn't delineate
        # the failures and successes properly.
        fail_markers = []
        success_markers = []
        for idx, line in enumerate(results):
            if line == FAILURE_MARKER:
                if len(success_markers) > 0:
                    success_markers.append(idx)
                else:
                    fail_markers.append(idx)
            if line.endswith("items passed all tests:"):
                success_markers.append(idx)
                fail_markers.append(idx)
        # Parse failure text first as the last section can contain
        # information about other tests that have passed.
        nmarkers = len(fail_markers)
        failcases = []
        for i in range(0, nmarkers - 1):
            start, end = fail_markers[i] + 1, fail_markers[i+1]
            failcases.append(self.__create_failure_report(classname,
                                                          results[start:end]))

        if len(success_markers) == 0:
            return failcases
        # Parse successful tests that have unique names
        start, end = success_markers
        passcases = self.__parse_success(fullname, results[start:end])
        # The final puzzle piece is that some tests that have failed
        # may have the same names as those that have passed.
        for line in results[end+1:]:
            match = MIX_FAIL_RE.match(line)
            if not match:
                continue
            nfails, ntotal = int(match.group(1)), int(match.group(2))
            npasses = ntotal - nfails
            name = match.group(3)
            for i in range(npasses):
                passcases.append(TestCaseReport(classname, name,
                                                failure_descr=None))

        return failcases + passcases

    def __create_classname(self, fullname):
        """
        Given a fullname, that can include path separators,
        produce a classname for the document

        Args:
          fullname (str): Fullname of document (including paths)
        """
        return PACKAGE_NAME + "." + fullname

    def __create_failure_report(self, classname, failure_desc):
        """
        Create a TestCaseReport from a description of the failure. The
        name is retrieved from the end of first line of description.

        Args:
          classname (str): A string name for the 'class'
          failure_desrc (list): A list of lines giving describing the failure

        Returns:
          A new TestCaseReport object
        """
        match = FAILURE_LOC_RE.match(failure_desc[0])
        if not match:
            raise ValueError("Unexpected failure description format.\n"
                             "Expected the first line to contain details "
                             "of the location of the error.\n"
                             "Found '%s'" % failure_desc[0])
        name = match.group(3)
        return TestCaseReport(classname, name, "\n".join(failure_desc))

#-------------------------------------------------------------------------------

def doctest_to_xunit(app, exception):
    """
    If the runner was 'doctest'then parse the "output.txt"
    file and produce an XUnit-style XML file, otherwise it does
    nothing.

    Arguments:
      app (Sphinx.application): Sphinx application object
      exception: (Exception): If an exception was raised then it is given here
    """
    if app.builder.name != "doctest":
        app.debug("Skipping xunit parsing for builder '%s'" % app.builder.name)
        return
    import os

    doctest_file = os.path.join(app.builder.outdir, DOCTEST_OUTPUT)
    app.debug("Parsing doctest output file '%s'" % doctest_file)
    doctests = DocTestOutputParser(doctest_file)
    app.debug("Saving doctest as xunit to file '%s'" % doctest_file)
    xunit_file = os.path.join(app.builder.outdir, XUNIT_OUTPUT)

    doctests.as_xunit(xunit_file)

#-------------------------------------------------------------------------------

def setup(app):
    """
    Connect the 'build-finished' event to the handler function.
    """
    app.connect('build-finished', doctest_to_xunit)
