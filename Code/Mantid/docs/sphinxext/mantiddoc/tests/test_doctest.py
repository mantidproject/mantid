"""
    Tests for the doctest addons
"""
from mantiddoc.doctest import DocTestOutputParser, TestCaseReport, TestSuiteReport

import unittest

class TestCaseReportTest(unittest.TestCase):

    def test_report_stores_expected_attributes_about_test(self):
        name = "DummyTest"
        classname = "DummySuite"
        failure_txt = "Test failed"
        report = TestCaseReport(classname, name, failure_txt)

        self.assertEquals(name, report.name)
        self.assertEquals(classname, report.classname)
        self.assertEquals(failure_txt, report.failure_descr)

    def test_case_passed_with_empty_failure_description(self):
        name = "DummyTest"
        classname = "DummySuite"
        failure_txt = ""
        report = TestCaseReport(classname, name, failure_txt)

        self.assertTrue(report.passed)
        self.assertFalse(report.failed)

    def test_case_passed_with_failure_description_as_None(self):
        name = "DummyTest"
        classname = "DummySuite"
        failure_txt = None
        report = TestCaseReport(classname, name, failure_txt)

        self.assertTrue(report.passed)
        self.assertFalse(report.failed)

    def test_case_failed_with_non_empty_failure_description(self):
        name = "DummyTest"
        classname = "DummySuite"
        failure_txt = "Test failed"
        report = TestCaseReport(classname, name, failure_txt)

        self.assertTrue(report.failed)
        self.assertFalse(report.passed)

#------------------------------------------------------------------------------

class TestSuiteReportTest(unittest.TestCase):

    def test_report_stores_expected_attributes_about_test(self):
        name = "DummySuite"
        package = "tests"
        testcases = [TestCaseReport("doctests", "DummyTest", "failed")]
        report =  TestSuiteReport(name, testcases, package)

        self.assertEquals(name, report.name)
        self.assertEquals(package, report.package)
        self.assertEquals(testcases, report.testcases)

    def test_report_gives_correct_number_test_passed_and_failed(self):
        report = self.__createDummyReport()

        self.assertEquals(1, report.npassed)
        self.assertEquals(1, report.nfailed)
        self.assertEquals(2, report.ntests)

    #========================= Failure cases ==================================

    def test_report_raises_error_with_empty_tests_cases_list(self):
        self.assertRaises(ValueError, self.__createDummyReport, empty = True)

    #========================= Helpers ========================================

    def __createDummyReport(self, empty = False):
        name = "DummySuite"
        package = "tests"
        if empty:
            testcases = []
        else:
            testcases = [TestCaseReport("doctests", "DummyTest", "failed"),
                         TestCaseReport("doctests", "DummyTest2", "")]

        return TestSuiteReport(name, testcases, package)

#------------------------------------------------------------------------------

ALL_PASS_EX = \
"""
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
"""

ALL_FAIL_EX = \
"""Document: algorithms/AllFailed
------------------------------
**********************************************************************
File "algorithms/AllFailed.rst", line 127, in Ex2
Failed example:
    print "Multi-line failed"
    print "test"
Expected:
    No match
Got:
    Multi-line failed
    test
**********************************************************************
File "algorithms/AllFailed.rst", line 111, in Ex1
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
"""

MIX_PASSFAIL_EX = \
"""Document: algorithms/MixPassFail
--------------------------------
**********************************************************************
File "algorithms/MixPassFail.rst", line 143, in default
Failed example:
    print "A failed test"
Expected:
    Not a success
Got:
    A failed test
**********************************************************************
File "algorithms/MixPassFail.rst", line 159, in Ex1
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

class DocTestOutputParserTest(unittest.TestCase):

    def test_all_passed_gives_expected_results(self):
        parser = DocTestOutputParser(ALL_PASS_EX, isfile = False)

        self.assertTrue(hasattr(parser, "testsuite"))
        suite = parser.testsuite
        self.assertEquals("doctests", suite.name)
        self.assertEquals("docs", suite.package)
        self.assertEquals(3, suite.ntests)

        cases = suite.testcases
        expected_names = ["Ex 2", "default", "default"]
        for idx, case in enumerate(cases):
            self.assertTrue(case.passed)
            self.assertEquals(expected_names[idx], case.name)
            self.assertEquals("algorithms.AllPassed", case.classname)

    def test_all_failed_gives_expected_results(self):
        parser = DocTestOutputParser(ALL_FAIL_EX, isfile = False)

        self.assertTrue(hasattr(parser, "testsuite"))
        suite = parser.testsuite
        self.assertEquals("doctests", suite.name)
        self.assertEquals("docs", suite.package)
        self.assertEquals(2, suite.ntests)

        cases = suite.testcases
        expected_names = ["Ex2", "Ex1"]
        expected_errors = [
"""File "algorithms/AllFailed.rst", line 127, in Ex2
Failed example:
    print "Multi-line failed"
    print "test"
Expected:
    No match
Got:
    Multi-line failed
    test""", # second error
"""File "algorithms/AllFailed.rst", line 111, in Ex1
Failed example:
    print "Single line failed test"
Expected:
    No match
Got:
    Single line failed test"""
]
        # test
        for idx, case in enumerate(cases):
            self.assertTrue(case.failed)
            self.assertEquals(expected_names[idx], case.name)
            self.assertEquals(expected_errors[idx], case.failure_descr)
            self.assertEquals("algorithms.AllFailed", case.classname)

    def test_mix_pass_fail_gives_expected_results(self):
        parser = DocTestOutputParser(MIX_PASSFAIL_EX, isfile = False)

        self.assertTrue(hasattr(parser, "testsuite"))
        suite = parser.testsuite
        self.assertEquals("doctests", suite.name)
        self.assertEquals("docs", suite.package)
        self.assertEquals(4, suite.ntests)

        cases = suite.testcases
        expected_names = ["default", "Ex1", "Ex3", "default"]
        expected_errors = [
"""File "algorithms/MixPassFail.rst", line 143, in default
Failed example:
    print "A failed test"
Expected:
    Not a success
Got:
    A failed test""",
"""File "algorithms/MixPassFail.rst", line 159, in Ex1
Failed example:
    print "Second failed test"
Expected:
    Not a success again
Got:
    Second failed test""",
"", "" #two passes
]
        # test
        for idx, case in enumerate(cases):
            expected_fail = True
            if expected_errors[idx] == "":
                expected_fail = False
            self.assertEquals(expected_fail, case.failed)
            self.assertEquals(expected_names[idx], case.name)
            self.assertEquals(expected_errors[idx], case.failure_descr)
            self.assertEquals("algorithms.MixPassFail", case.classname)

    #========================= Failure cases ==================================

    def test_no_document_start_gives_valueerror(self):
        self.assertRaises(ValueError, DocTestOutputParser,
                          "----------\n 1 items passed", isfile = False)

    def test_no_location_for_test_failure_gives_valueerror(self):
        fail_ex_noloc = ALL_FAIL_EX.splitlines()
        #remove the location line
        fail_ex_noloc.pop(3)
        fail_ex_noloc = "\n".join(fail_ex_noloc)

        self.assertRaises(ValueError, DocTestOutputParser, fail_ex_noloc,
                          isfile = False)

    def test_no_overall_summary_for_document_gives_valueerror(self):
        fail_ex_nosum = ALL_FAIL_EX.splitlines()
        fail_ex_nosum.pop(26)
        fail_ex_nosum = "\n".join(fail_ex_nosum)

        self.assertRaises(ValueError, DocTestOutputParser, fail_ex_nosum,
                          isfile = False)

#------------------------------------------------------------------------------

if __name__ == '__main__':
    unittest.main()
