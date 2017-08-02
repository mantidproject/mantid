#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from Muon import test
from PyQt4 import QtGui
import sys


def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app

app = qapp()

ex =test.Form() #the main ui class in this file is called MainWindow
ex.resize(700,700)
ex.show()
app.exec_()
