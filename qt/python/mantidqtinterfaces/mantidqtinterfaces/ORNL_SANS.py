# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
"""
Script used to start the HFIR SANS reduction gui from Mantidplot
"""

import sys
from mantidqtinterfaces.reduction_application import ReductionGUI

if "workbench" in sys.modules:
    from workbench.config import get_window_config

    parent, flags = get_window_config()
else:
    parent, flags = None, None
reducer = ReductionGUI(parent, flags, instrument_list=["BIOSANS", "GPSANS", "EQSANS"])
if reducer.setup_layout(load_last=True):
    reducer.show()
