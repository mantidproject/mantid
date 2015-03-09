#pylint: disable=invalid-name
"""
    Script used to start the DGS reduction GUI from MantidPlot
"""
import sys

from HFIRPowderReduction import HfirPDReductionGUI
from PyQt4 import QtGui

def qapp():
    if QtGui.QApplication.instance():
        _app = QtGui.QApplication.instance()
    else:
        _app = QtGui.QApplication(sys.argv)
    return _app

app = qapp()

reducer = HfirPDReductionGUI.MainWindow() #the main ui class in this file is called MainWindow
reducer.show()

app.exec_()
