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

    def test_report_gives_corret_number_test_passed_and_failed(self):
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
            self.assertEquals("AllPassed", case.classname)

    #========================= Failure cases ==================================

    def test_no_document_start_gives_valueerror(self):
        self.assertRaises(ValueError, DocTestOutputParser,
                          "----------\n 1 items passed", isfile = False)

#------------------------------------------------------------------------------

if __name__ == '__main__':
    unittest.main()

