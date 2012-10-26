"""
    Script used to start the ISIS Reflectomery GUI from MantidPlot
"""
from PyQt4 import QtGui
from isis_reflgui import reflgui
MainWindow = QtGui.QMainWindow()
ui = reflgui.Ui_MainWindow()
ui.setupUi(MainWindow)
MainWindow.show()
