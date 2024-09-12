# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
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

        self.assertEqual(name, report.name)
        self.assertEqual(classname, report.classname)
        self.assertEqual(failure_txt, report.failure_descr)

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


class TestSuiteReportTest(unittest.TestCase):
    def test_report_stores_expected_attributes_about_test(self):
        name = "DummySuite"
        package = "tests"
        testcases = [TestCaseReport("doctests", "DummyTest", "failed")]
        report = TestSuiteReport(name, testcases, package)

        self.assertEqual(name, report.name)
        self.assertEqual(package, report.package)
        self.assertEqual(testcases, report.testcases)

    def test_report_gives_correct_number_test_passed_and_failed(self):
        report = self.__createDummyReport()

        self.assertEqual(1, report.npassed)
        self.assertEqual(1, report.nfailed)
        self.assertEqual(2, report.ntests)

    def test_report_raises_error_with_empty_tests_cases_list(self):
        self.assertRaises(ValueError, self.__createDummyReport, empty=True)

    def __createDummyReport(self, empty=False):
        name = "DummySuite"
        package = "tests"
        if empty:
            testcases = []
        else:
            testcases = [TestCaseReport("doctests", "DummyTest", "failed"), TestCaseReport("doctests", "DummyTest2", "")]

        return TestSuiteReport(name, testcases, package)


ALL_PASS_EX = """
Document: algorithms/AllPassed
------------------------------
2 items passed all tests:
   1 tests in Ex 2
   2 tests in default
3 tests in 2 items.
3 passed and 0 failed.
Test passed.
1 items passed all tests:
   1 tests in Ex (cleanup code)
1 tests in 1 items.
1 passed and 0 failed.
Test passed.

Doctest summary
===============
3 tests
0 failures in tests
0 failures in setup code
0 failures in cleanup code
"""

TEST_PASS_CLEANUP_FAIL = """
Document: algorithms/TestPassedCleanupFail
------------------------------------------
**********************************************************************
File "algorithms/AllPassed.rst", line 64, in default (cleanup code)
Failed example:
    failed
Exception raised:
    Traceback (most recent call last):
      File "/usr/lib/python2.7/doctest.py", line 1289, in __run
        compileflags, 1) in test.globs
      File "<doctest default (cleanup code)[0]>", line 1, in <module>
        failed
    NameError: name 'failed' is not defined
2 items passed all tests:
   1 tests in Ex 2
   2 tests in default
3 tests in 2 items.
3 passed and 0 failed.
Test passed.
**********************************************************************
1 items had failures:
   1 of   1 in default (cleanup code)
1 tests in 1 items.
0 passed and 1 failed.
***Test Failed*** 1 failures.

Doctest summary
===============
3 tests
0 failures in tests
1 failures in setup code
0 failures in cleanup code
"""

ALL_FAIL_EX = """Document: algorithms/AllFailed
------------------------------
**********************************************************************
File "algorithms/AllFailed.rst", line 127, in Ex2[31]
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
   1 of   1 in Ex2[31]
2 tests in 2 items.
0 passed and 2 failed.
***Test Failed*** 2 failures.
2 items passed all tests:
   1 tests in Ex1 (cleanup code)
   1 tests in Ex2[31] (cleanup code)
2 tests in 2 items.
2 passed and 0 failed.
Test passed.

Doctest summary
===============
    2 tests
    2 failures in tests
    0 failures in setup code
    0 failures in cleanup code
"""

MIX_PASSFAIL_EX = """Document: algorithms/MixPassFail
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
1 items passed all tests:
   1 tests in Ex (cleanup code)
1 tests in 1 items.
1 passed and 0 failed.
Test passed.

Doctest summary
===============
    4 tests
    2 failures in tests
    0 failures in setup code
    0 failures in cleanup code
"""


class DocTestOutputParserTest(unittest.TestCase):
    def test_all_passed_gives_expected_results(self):
        parser = DocTestOutputParser(ALL_PASS_EX, isfile=False)

        self.assertTrue(hasattr(parser, "testsuite"))
        suite = parser.testsuite
        self.assertEqual("doctests", suite.name)
        self.assertEqual("docs", suite.package)
        self.assertEqual(3, suite.ntests)

        cases = suite.testcases
        expected_names = ["Ex 2", "default", "default"]
        for idx, case in enumerate(cases):
            self.assertTrue(case.passed)
            self.assertEqual(expected_names[idx], case.name)
            self.assertEqual("docs.algorithms/AllPassed", case.classname)

    def test_pass_with_cleanup_fail_parse_correctly(self):
        parser = DocTestOutputParser(TEST_PASS_CLEANUP_FAIL, isfile=False)

        self.assertTrue(hasattr(parser, "testsuite"))
        suite = parser.testsuite
        self.assertEqual("doctests", suite.name)
        self.assertEqual("docs", suite.package)
        self.assertEqual(3, suite.ntests)

        cases = suite.testcases
        expected_names = ["Ex 2", "default", "default"]
        expected_pass = [True, False, True]
        for idx, case in enumerate(cases):
            self.assertEqual(expected_pass[idx], case.passed)
            self.assertEqual(expected_names[idx], case.name)
            self.assertEqual("docs.algorithms/TestPassedCleanupFail", case.classname)

    def test_all_failed_gives_expected_results(self):
        parser = DocTestOutputParser(ALL_FAIL_EX, isfile=False)

        self.assertTrue(hasattr(parser, "testsuite"))
        suite = parser.testsuite
        self.assertEqual("doctests", suite.name)
        self.assertEqual("docs", suite.package)
        self.assertEqual(2, suite.ntests)

        cases = suite.testcases
        expected_names = ["Ex2[31]", "Ex1"]
        expected_errors = [
            """File "algorithms/AllFailed.rst", line 127, in Ex2[31]
Failed example:
    print "Multi-line failed"
    print "test"
Expected:
    No match
Got:
    Multi-line failed
    test""",
            """File "algorithms/AllFailed.rst", line 111, in Ex1
Failed example:
    print "Single line failed test"
Expected:
    No match
Got:
    Single line failed test""",
        ]
        # test
        for idx, case in enumerate(cases):
            self.assertTrue(case.failed)
            self.assertEqual(expected_names[idx], case.name)
            self.assertEqual(expected_errors[idx], case.failure_descr)
            self.assertEqual("docs.algorithms/AllFailed", case.classname)

    def test_mix_pass_fail_gives_expected_results(self):
        parser = DocTestOutputParser(MIX_PASSFAIL_EX, isfile=False)

        self.assertTrue(hasattr(parser, "testsuite"))
        suite = parser.testsuite
        self.assertEqual("doctests", suite.name)
        self.assertEqual("docs", suite.package)
        self.assertEqual(4, suite.ntests)

        cases = suite.testcases
        expected_names = ["Ex3", "default", "default", "Ex1"]
        expected_errors = [
            "",
            "",
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
        ]
        # test
        for idx, case in enumerate(cases):
            expected_fail = expected_errors[idx] != ""
            self.assertEqual(expected_fail, case.failed)
            self.assertEqual(expected_names[idx], case.name)
            self.assertEqual(expected_errors[idx], case.failure_descr)
            self.assertEqual("docs.algorithms/MixPassFail", case.classname)

    def test_multi_document_text(self):
        multi_doc = "\n".join(ALL_PASS_EX.splitlines()[:-6])  # hack off summary
        multi_doc += ALL_FAIL_EX
        parser = DocTestOutputParser(multi_doc, isfile=False)

        self.assertTrue(hasattr(parser, "testsuite"))
        suite = parser.testsuite
        # The other checks should be sufficient if this passes
        self.assertEqual(5, suite.ntests)

    def test_no_document_start_gives_valueerror(self):
        self.assertRaises(ValueError, DocTestOutputParser, "----------\n 1 items passed", isfile=False)

    def test_no_location_for_test_failure_gives_valueerror(self):
        fail_ex_noloc = ALL_FAIL_EX.splitlines()
        # remove the location line
        fail_ex_noloc.pop(3)
        fail_ex_noloc = "\n".join(fail_ex_noloc)

        self.assertRaises(ValueError, DocTestOutputParser, fail_ex_noloc, isfile=False)

    def test_no_overall_summary_for_document_gives_valueerror(self):
        fail_ex_nosum = ALL_FAIL_EX.splitlines()
        fail_ex_nosum.pop(26)
        fail_ex_nosum = "\n".join(fail_ex_nosum)

        self.assertRaises(ValueError, DocTestOutputParser, fail_ex_nosum, isfile=False)


if __name__ == "__main__":
    unittest.main()
