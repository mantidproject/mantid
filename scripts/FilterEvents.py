# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from FilterEvents import eventFilterGUI
from PyQt4 import QtGui
import sys


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app


app = qapp()

reducer = eventFilterGUI.MainWindow() #the main ui class in this file is called MainWindow
reducer.show()
app.exec_()
