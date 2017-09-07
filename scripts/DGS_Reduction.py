#pylint: disable=invalid-name
"""
    Script used to start the DGS reduction GUI from MantidPlot
"""
from __future__ import (absolute_import, division, print_function)
from reduction_application import ReductionGUI

reducer = ReductionGUI(instrument_list=["ARCS", "CNCS", "HYSPEC", "MAPS",
                                        "MARI", "MERLIN", "SEQUOIA", "TOFTOF"])
if reducer.setup_layout(load_last=True):
    reducer.show()
