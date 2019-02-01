# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
"""
    Script used to start the HFIR SANS reduction gui from Mantidplot
"""
from __future__ import (absolute_import, division, print_function)
from reduction_application import ReductionGUI

reducer = ReductionGUI(instrument_list=["BIOSANS", "GPSANS", "EQSANS"])
if reducer.setup_layout(load_last=True):
    reducer.show()
