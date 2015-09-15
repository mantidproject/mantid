"""
Utility functions for running python test scripts
inside MantidPlot.

Public methods:
    runTests(): to run MantidPlot unit tests
    screenshot(): take a screenshot and save to a report

"""
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
    print "QTest not available"

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
    # Get the XML runner if the environment variable was set
    src = os.getenv('MANTID_SOURCE')
    if src is None:
        runner = unittest.TextTestRunner()
    else:
        sys.path.append( os.path.join(src, "Testing", "Tools", "unittest-xml-reporting", "src") )
        import xmlrunner
        runner = xmlrunner.XMLTestRunner(output='Testing')

    #Run using either runner
    res = runner.run(suite)

    # Process some events that ensure MantidPlot closes properly.
    QtCore.QCoreApplication.processEvents()
    QtCore.QCoreApplication.processEvents()
    QtCore.QCoreApplication.processEvents()

    # Set Mantid exit code
    if not res.wasSuccessful():
        _qti.app.setExitCode(1)
    else:
        _qti.app.setExitCode(0)

    return res
