"""
    Script used to start the HFIR SANS reduction gui from Mantidplot
"""
#from reduction_gui.reduction_application import ReductionGUI
from Interface.reduction_application import ReductionGUI
from PyQt4 import QtCore, uic

reducer = ReductionGUI(instrument_list=["BIOSANS", "EQSANS"])
reducer.setup_layout()
reducer.show()