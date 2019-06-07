# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""Provides a runner to execute unit tests with a given runner.
It basically sets the sip API to version 2. We can get rid of this
once qtpy is universally used.

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

    # Load the test and copy over any module variables so that we have
    # the same environment defined here
    test_module = imp.load_source(module_name(pathname), pathname)
    test_module_globals = dir(test_module)
    this_globals = globals()
    for key in test_module_globals:
        this_globals[key] = getattr(test_module, key)

    # create runner & execute
    unittest.main(
        module=test_module,
        # We've processed the test source so don't let unittest try to reparse it
        # This forces it to load the tests from the supplied module
        argv=(argv[0],),
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
