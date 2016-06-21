#pylint: disable=invalid-name
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
