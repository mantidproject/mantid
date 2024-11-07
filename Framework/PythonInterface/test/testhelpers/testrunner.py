# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Provides a runner to execute unit tests with a given runner.
It basically sets the sip API to version 2. We can get rid of this
once qtpy is universally used.

It is intended to be used as a launcher script for a given unit test file.
The reports are output to the current working directory.
"""

import importlib.util
from importlib.machinery import SourceFileLoader
import os
import sys
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

    # Load the test and copy over any module variables so that we have
    # the same environment defined here

    test_module_name = module_name(pathname)
    test_loader = SourceFileLoader(test_module_name, pathname)
    test_spec = importlib.util.spec_from_loader(test_module_name, test_loader)
    test_module = importlib.util.module_from_spec(test_spec)
    test_loader.exec_module(test_module)
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
        failfast=False,
        buffer=False,
        catchbreak=False,
    )


def module_name(pathname):
    """
    Returns a Python module name for the given pathname using the standard rules
    :param pathname: Path to a python file
    :return: A module name to give to the import mechanism
    """
    return os.path.splitext(os.path.basename(pathname))[0]


if __name__ == "__main__":
    main(sys.argv)
