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

# See inline comment about why we need this
TEST_OUTPUT_SUFFIX = 'unit'


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
    # The default output suffix is a timestamp so each run will generate a new file
    # This is inconsistent with how our other tests run so we want to suppress it
    # but old versions of xmlrunner check `if outputsuffix` and empty string
    # is the same as None in this case so we need a non-zero length string
    runner = xmlrunner.XMLTestRunner(output='.', outsuffix=TEST_OUTPUT_SUFFIX)
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
    Returns a Python module name for the given pathname
    :param pathname: Path to a python file
    :return: A module name to give to the import mechanism
    """
    return os.path.splitext(os.path.basename(pathname))[0]


if __name__ == "__main__":
    main(sys.argv)
