#pylint: disable=invalid-name
"""
    Script used to start the REFL reduction gui from Mantidplot
"""
from reduction_application import ReductionGUI

reducer = ReductionGUI(instrument="REFL", instrument_list=["REFL"])
if reducer.setup_layout(load_last=True):
    reducer.show()
