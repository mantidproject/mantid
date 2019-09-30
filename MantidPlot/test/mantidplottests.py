# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Utility functions for running python test scripts
inside MantidPlot.

Public methods:
    runTests(): to run MantidPlot unit tests
    screenshot(): take a screenshot and save to a report

"""
from __future__ import (absolute_import, division, print_function)
import sys
import os
import unittest
import time
import _qti
import datetime
from PyQt4 import QtGui, QtCore
from PyQt4.QtCore import pyqtSlot

from pymantidplot.proxies import threadsafe_call

# Try to import QTest. Not available on Windows?
try:
    from PyQt4.QtTest import QTest
    qtest = True
except:
    qtest = False
    print("QTest not available")

def moveMouseToCentre(widget):
    """Moves the mouse over the widget
    """
    if qtest:
        QtCore.QCoreApplication.processEvents()
        threadsafe_call(QTest.mouseMove, widget)
        QtCore.QCoreApplication.processEvents()

def runTests(classname):
    """ Run the test suite in the class.
    Uses the XML runner if the MANTID_SOURCE environment variable was set.
    """
    # Custom code to create and run this single test suite
    suite = unittest.TestSuite()
    suite.addTest( unittest.makeSuite(classname) )
    runner = unittest.TextTestRunner()

    #Run using either runner
    res = runner.run(suite)

    # Process some events that ensure MantidPlot closes properly.
    QtCore.QCoreApplication.processEvents()
    QtCore.QCoreApplication.processEvents()
    QtCore.QCoreApplication.processEvents()

    # Close Mantid and set exit code
    if not res.wasSuccessful():
        sys.exit(1)
    else:
        sys.exit(0)

    return res
