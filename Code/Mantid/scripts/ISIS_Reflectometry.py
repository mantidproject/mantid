"""
    Script used to start the ISIS Reflectomery GUI from MantidPlot
"""
from PyQt4 import QtGui
from Reflectometry.reflgui import *
MainWindow = QtGui.QMainWindow()
ui = Ui_MainWindow()
ui.setupUi(MainWindow)
MainWindow.show()
