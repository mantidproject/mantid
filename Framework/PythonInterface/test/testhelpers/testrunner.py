"""Provides a runner to execute unit tests with a given runner

It is intended to be used as a launcher script for a given unit test file.
The reports are output to the current working directory.
"""
from __future__ import (absolute_import, division, print_function)

import imp
import os
import sys
import xmlrunner
import unittest


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

    # Load the test
    test_module = imp.load_source(module_name(pathname), pathname)
    runner = xmlrunner.XMLTestRunner(output='.', outsuffix='')
    # execute
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
    Returns a Python module name for the given pathname. This is used to form the suite name
    for the test when written as xUnit-style output. We fake one so that we can group the tests
    more easily. The parent's directory name is used as a namespace
    :param pathname: Path to a python file
    :return: A module name to give to the import mechanism
    """
    directory_path, basename = os.path.split(pathname)
    directory_name = os.path.relpath(directory_path, os.path.dirname(directory_path))
    test_filename, _ = os.path.splitext(basename)
    return 'python.' + os.path.basename(directory_name) + "." + test_filename


if __name__ == "__main__":
    main(sys.argv)
