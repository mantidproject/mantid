# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import sys
import unittest
from PyQt4 import QtCore, QtGui
import mantid
import mantidqtpython

# Create the application only once per test; otherwise I get a segfault
app = QtGui.QApplication(sys.argv)


class MWRunFilesTest(unittest.TestCase):
    """Test for accessing SliceViewer widgets from MantidPlot
    python interpreter"""

    def setUp(self):
        self.mwrunfiles = mantidqtpython.MantidQt.API.MWRunFiles()

    def tearDown(self):
        """ Close the created widget """
        # This is crucial! Forces the object to be deleted NOW, not when python exits
        # This prevents a segfault in Ubuntu 10.04, and is good practice.
        self.mwrunfiles.deleteLater()
        # Schedule quit at the next event
        QtCore.QTimer.singleShot(0, app, QtCore.SLOT("quit()"))
        # This is required for deleteLater() to do anything (it deletes at the next event loop)
        app.quitOnLastWindowClosed = True
        app.exec_()

    def test_creation(self):
        self.assertTrue(isinstance(self.mwrunfiles,mantidqtpython.MantidQt.API.MWRunFiles), "Created object is not an instance of MWRunFiles")

    def test_lineedit_text(self):
        self.assertEqual(len(self.mwrunfiles.text()), 0)
        self.mwrunfiles.setText("a/file")
        self.assertEqual(self.mwrunfiles.text(), "a/file")

    def test_setUserInput(self):
        self.assertFalse(self.mwrunfiles.isValid())
        self.mwrunfiles.setUserInput("CNCS7860")
        self.assertEqual(self.mwrunfiles.text(), "CNCS7860")

if __name__ == '__main__':
    unittest.main()
