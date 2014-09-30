from TofConverter import converterGUI
from PyQt4 import QtGui
import sys

def qapp():
    if QtGui.QApplication.instance():
    	app = QtGui.QApplication.instance()
    else:
    	app = QtGui.QApplication(sys.argv)
    return app

app = qapp()
reducer = converterGUI.MainWindow()#the main ui class in this file is called MainWindow
reducer.show()
app.exec_()