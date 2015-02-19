"""
    Script used to start the REFL reduction gui from Mantidplot
"""
from reduction_application import ReductionGUI
from PyQt4 import QtCore, uic

reducer = ReductionGUI(instrument="REFL", instrument_list=["REFL"])
if reducer.setup_layout(load_last=True):
    reducer.show()
