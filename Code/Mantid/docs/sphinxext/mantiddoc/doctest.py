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

#-------------------------------------------------------------------------------
# Define parts of lines that denote a document
DOCTEST_DOCUMENT_BEGIN = "Document:"
DOCTEST_SUMMARY_TITLE = "Doctest summary"

# Regexes
ALLPASS_TEST_NAMES_RE = re.compile(r"^\s+(\d+) tests in (.+)$")
NUMBER_PASSED_RE = re.compile(r"^(\d+) items passed all tests:$")

TEST_FAILED_END_RE = re.compile(r"\*\*\*Test Failed\*\*\* (\d+) failures.")
FAILURE_LOC_RE = re.compile(r'^File "([\w/\.]+)", line (\d+), in (\w+)$')

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

    def __init__(self, doctest_output, isfile):
        """
        Parses the given doctest output

        Args:
          doctest_output (str): String giving either doctest output as plain
                                text or a filename
          isfile (bool): If True then the doctest_output argument is treated
                          as a filename
        """
        if isfile:
            with open(filename,'r') as result_file:
                self.testsuite = self.__parse(result_file)
        else:
            self.testsuite = self.__parse(doctest_output.splitlines())

    def as_xunit(self, filename):
        """
        Write out the test results in Xunit-style format
        """
        cases = self.testsuite.testcases
        suite_node = ElementTree.Element("testsuite")
        suite_node.attrib["name"] = self.testsuite.name
        suite_node.attrib["tests"] = str(self.testsuite.ntests)
        suite_node.attrib["failures"] = str(self.testsuite.nfailures)
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
                               package="docs")

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
        Parse text for success cases for a single document

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

        cases = []
        failure_desc = []
        indoc = False
        for line in results:
            if line.startswith("*"):
                if len(cases) == nfailed:
                    break
                if len(failure_desc) > 0:
                    cases.append(self.__create_failure_report(classname,
                                                              failure_desc))
                failure_desc = []
                indoc = True
                continue

            if indoc:
                failure_desc.append(line)

        return cases

    def __create_classname(self, fullname):
        """
        Given a fullname, that can include path separators,
        produce a classname for the document

        Args:
          fullname (str): Fullname of document (including paths)
        """
        return fullname.replace("/", ".")

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
