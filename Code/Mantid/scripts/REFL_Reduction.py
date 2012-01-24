"""
    Script used to start the REFL reduction gui from Mantidplot
"""
#from reduction_gui.reduction_application import ReductionGUI
from Interface.reduction_application import ReductionGUI
from PyQt4 import QtCore, uic

reducer = ReductionGUI(instrument_list=["REFL"])
reducer.setup_layout(load_last=True)
reducer.show()