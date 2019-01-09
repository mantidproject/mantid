# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as simple
import os


def focus_whole(van_curves, van_int, run_number):
    ws_to_focus = simple.Load(Filename="ENGINX" + run_number, OutputWorkspace="engg_focus_input")
    van_integrated_ws = simple.Load(Filename=van_int)
    van_curves_ws = simple.Load(Filename=van_curves)
    for i in range(1, 3):
        simple.EnggFocus(InputWorkspace=ws_to_focus, OutputWorkspace="engg_focus_output_bank_" + str(i),
                         VanIntegrationWorkspace=van_integrated_ws, VanCurvesWorkspace=van_curves_ws,
                         Bank=str(i))
    print("done")


def focus_cropped(use_spectra, crop_on, van_curves, van_int, run_number):
    ws_to_focus = simple.Load(Filename="ENGINX" + run_number, OutputWorkspace="engg_focus_input")
    van_integrated_ws = simple.Load(Filename=van_int)
    van_curves_ws = simple.Load(Filename=van_curves)
    if not use_spectra:
        bank = {"North": "1",
                "South": "2"}
        simple.EnggFocus(InputWorkspace=ws_to_focus, OutputWorkspace="engg_focus_output_bank_" + bank.get(crop_on),
                         VanIntegrationWorkspace=van_integrated_ws, VanCurvesWorkspace=van_curves_ws,
                         Bank=bank.get(crop_on))
    else:
        simple.EnggFocus(InputWorkspace=ws_to_focus, OutputWorkspace="engg_focus_output",
                         VanIntegrationWorkspace=van_integrated_ws,
                         VanCurvesWorkspace=van_curves_ws, SpectrumNumbers=crop_on)
