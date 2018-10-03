# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from HFIR_4Circle_Reduction import reduce4circleGUI
from PyQt4 import QtGui, QtCore
import sys


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app


# try to defeat X11 unsafe thread
QtCore.QCoreApplication.setAttribute(QtCore.Qt.AA_X11InitThreads)
app = qapp()

reducer = reduce4circleGUI.MainWindow() #the main ui class in this file is called MainWindow
reducer.show()
app.exec_()
