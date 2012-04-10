"""
    Script used to start the REFL SF calculator gui from Mantidplot
"""
#from reduction_gui.reduction_application import ReductionGUI
from Interface.reduction_application import ReductionGUI
from PyQt4 import QtCore, uic

reducer = ReductionGUI(instrument="REFLSF", instrument_list=["REFLSF"])
reducer.setup_layout(load_last=True)
reducer.show()