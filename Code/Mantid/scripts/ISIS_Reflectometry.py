"""
    Script used to start the ISIS Reflectomery GUI from MantidPlot
"""
from PyQt4 import QtGui
from ui.reflectometer import refl_window
MainWindow = QtGui.QMainWindow()
ui = refl_window.ReflGuiWindow()
ui.setupUi(MainWindow)
MainWindow.show()
