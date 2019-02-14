# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""Provides a runner to execute unit tests with a given runner

It is intended to be used as a launcher script for a given unit test file.
The reports are output to the current working directory.
"""
from __future__ import (absolute_import, division, print_function)

import imp
import os
import sys
import unittest

# If any tests happen to hit a PyQt4 import make sure item uses version 2 of the api
# Remove this when everything is switched to qtpy
import sip
try:
    sip.setapi('QString', 2)
    sip.setapi('QVariant', 2)
    sip.setapi('QDate', 2)
    sip.setapi('QDateTime', 2)
    sip.setapi('QTextStream', 2)
    sip.setapi('QTime', 2)
    sip.setapi('QUrl', 2)
except AttributeError:
    # PyQt < v4.6
    pass

from xmlrunner import XMLTestRunner
from xmlrunner.result import _TestInfo, _XMLTestResult, safe_unicode

class GroupedNameTestInfo(_TestInfo):
    """
    Overrides these default xmlrunner class to
    used a different test naming scheme
    """

    def __init__(self, test_result, test_method, outcome=_TestInfo.SUCCESS,
                 err=None, subTest=None):
        super(GroupedNameTestInfo, self).__init__(test_result, test_method, outcome,
                                                  err, subTest)

    def id(self):
        return self.test_id


class GroupedNameTestResult(_XMLTestResult):
    """
    A hack to allow us to prefix the test suite name with a prefix we choose allowing
    to output to be organized by Jenkins.
    """

    testcase_prefix = None

    def __init__(self, stream=sys.stderr, descriptions=1, verbosity=1,
                 elapsed_times=True, properties=None):
        super(GroupedNameTestResult, self).__init__(stream, descriptions, verbosity,
                                                    elapsed_times, properties,
                                                    infoclass=GroupedNameTestInfo)

    def _get_info_by_testcase(self):
        """
        Organizes test results by TestCase module. This information is
        used during the report generation, where a XML report will be created
        for each TestCase.
        """
        tests_by_testcase = {}

        if self.testcase_prefix is None:
            self.testcase_prefix = ""
        for tests in (self.successes, self.failures, self.errors,
                      self.skipped):
            for test_info in tests:
                if isinstance(test_info, tuple):
                    # This is a skipped, error or a failure test case
                    test_info = test_info[0]
                testcase_name = self.testcase_prefix + test_info.test_name
                if testcase_name not in tests_by_testcase:
                    tests_by_testcase[testcase_name] = []
                tests_by_testcase[testcase_name].append(test_info)

        return tests_by_testcase

def main(argv):
    """
    Runs the test files through the xml runner
    :param argv: List of command line arguments
    """
    if len(argv) != 2:
        raise ValueError("Usage: testrunner <path-to-test-file>")

    pathname = argv[1]
    if not os.path.exists(pathname):
        raise ValueError("Test file not found '{}'".format(pathname))
    if not os.path.isfile(pathname):
        raise ValueError("Test path '{}' is not a file".format(pathname))

    # Add the directory of the test to the Python path
    dirname = os.path.dirname(pathname)
    # if the directory ends with 'tests' add the parent directory as well
    # this is used in the external project PyStoG
    sys.path.insert(0, dirname)
    if os.path.split(dirname)[-1] == 'tests':
        sys.path.insert(1, os.path.split(dirname)[0])

    # Load the test and copy over any module variables so that we have
    # the same environment defined here
    test_module = imp.load_source(module_name(pathname), pathname)
    test_module_globals = dir(test_module)
    this_globals = globals()
    for key in test_module_globals:
        this_globals[key] = getattr(test_module, key)

    # create runner & execute
    runner = XMLTestRunner(output='.', outsuffix='', resultclass=result_class(pathname))
    unittest.main(
        module=test_module,
        # We've processed the test source so don't let unittest try to reparse it
        # This forces it to load the tests from the supplied module
        argv=(argv[0],),
        testRunner=runner,
        # these make sure that some options that are not applicable
        # remain hidden from the help menu.
        failfast=False, buffer=False, catchbreak=False
    )


def module_name(pathname):
    """
    Returns a Python module name for the given pathname using the standard rules
    :param pathname: Path to a python file
    :return: A module name to give to the import mechanism
    """
    return os.path.splitext(os.path.basename(pathname))[0]


def result_class(pathname):
    """
    Returns a result class that can be passed to XMLTestRunner that
    customizes the test naming to suite our needs. Note that this
    is only suitable for running tests from a single file.
    :return: A sub class of _XMLTestResult
    """
    directory_path, _ = os.path.split(pathname)
    directory_name = os.path.relpath(directory_path, os.path.dirname(directory_path))
    class_ = GroupedNameTestResult
    class_.testcase_prefix = "python." +  directory_name + "."
    return class_


if __name__ == "__main__":
    main(sys.argv)
