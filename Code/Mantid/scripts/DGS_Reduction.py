"""
    Script used to start the DGS reduction GUI from MantidPlot
"""
from reduction_application import ReductionGUI
from PyQt4 import QtCore, uic

reducer = ReductionGUI(instrument_list=["ARCS", "CNCS", "HYSPEC", "MAPS",
                                        "MARI", "MERLIN", "SEQUOIA"])
if reducer.setup_layout(load_last=True):
    reducer.show()
