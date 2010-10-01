"""
    Script used to start the HFIR SANS reduction gui from Mantidplot
"""
from reduction_gui.reduction_application import ReductionGUI
from PyQt4 import QtCore, uic

reducer = ReductionGUI()
    
f = QtCore.QFile(":/reduction_main.ui")
f.open(QtCore.QIODevice.ReadOnly)    
uic.loadUi(f, reducer)
reducer.setup_layout()
reducer.show()