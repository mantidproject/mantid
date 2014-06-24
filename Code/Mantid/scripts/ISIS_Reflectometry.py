"""
    Script used to start the ISIS Reflectomery GUI from MantidPlot
"""
from PyQt4 import QtGui
#, QtCore
from ui.reflectometer import refl_gui

ui = refl_gui.ReflGui()
if ui.setup_layout():
    ui.show()
