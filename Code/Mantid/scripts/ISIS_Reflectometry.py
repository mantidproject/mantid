"""
    Script used to start the ISIS Reflectomery GUI from MantidPlot
"""
from PyQt4 import QtGui
from ui.reflectometer import refl_gui
MainWindow = QtGui.QMainWindow()
ui = refl_gui.ReflGui()
ui.setupUi(MainWindow)
MainWindow.show()
