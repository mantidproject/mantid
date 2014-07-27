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

    Document: bar/FooDoc
    --------------------
    1 items passed all tests:
       2 tests in default
       1 tests in ExForFoo
    2 tests in 1 items.
    2 passed and 0 failed.
    Test passed.

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

"""

#-------------------------------------------------------------------------------
class TestSuite(object):

    def __init__(self, name, cases, package=None):
        self.name = name
        self.testcases = cases
        self.package = package

    @property
    def ntests(self):
        return len(self.testcases)

    @property
    def nfailures(self):
        def sum_failure(fails, case):
            if case.failed: return fails + 1
            else: return fails
        return reduce(sum_failure, self.testcases, 0)

    @property
    def npassed(self):
        return self.ntests - self.nfailures

#-------------------------------------------------------------------------------
class TestCase(object):

    def __init__(self, classname, name, failure_descr):
        self.classname = classname
        self.name = name
        self.failure_descr = failure_descr

    @property
    def passed(self):
        return (self.failure_descr is None)

    @property
    def failed(self):
        return not self.passed

#-------------------------------------------------------------------------------
class DocTestOutputParser(object):
    """
    Process a doctest output file and convert it
    to a different format
    """

    def __init__(self, filename):
        with open(filename,'r') as result_file:
            self.testsuite = self.__parse(result_file)

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

    def __parse(self, result_file):
        """
        Parse a doctest output file and a TestSuite
        object that describe the results of the
        all tests on a single document

        Arguments:
          result_file (File): File-like object

        Returns:
          TestSuite: TestSuite object
        """
        pass

#-------------------------------------------------------------------------------
