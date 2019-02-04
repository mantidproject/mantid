# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Utility functions for running python test scripts
inside Workbench.

Public methods:
    runTests(): to run Workbench unit tests

"""
from __future__ import (absolute_import, division, print_function)
import sys
import os
import unittest
from qtpy.QtCore import QCoreApplication
from workbench.plotting.qappthreadcall import QAppThreadCall


def runTests(classname):
    """ Run the test suite in the class.
    Uses the XML runner if the MANTID_SOURCE environment variable was set.
    """
    # Custom code to create and run this single test suite
    suite = QAppThreadCall(unittest.TestSuite)()
    QAppThreadCall(suite.addTest)(unittest.makeSuite(classname))
    # Get the XML runner if the environment variable was set
    src = os.getenv('MANTID_SOURCE')
    if src is None:
        runner = QAppThreadCall(unittest.TextTestRunner)()
    else:
        sys.path.append(os.path.join(src, "Testing", "Tools", "unittest-xml-reporting", "src"))
        import xmlrunner
        runner = QAppThreadCall(xmlrunner.XMLTestRunner)(output='Testing')

    # Run using either runner
    res = QAppThreadCall(runner.run)(suite)

    # Process some events that ensure MantidPlot closes properly.
    QCoreApplication.processEvents()
    QCoreApplication.processEvents()
    QCoreApplication.processEvents()

    # Close Mantid and set exit code
    if not res.wasSuccessful():
        sys.exit(1)
    else:
        sys.exit(0)
