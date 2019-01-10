# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as simple
import os


def focus_whole(van_curves, van_int, run_number, focus_dir):
    ws_to_focus = simple.Load(Filename="ENGINX" + run_number, OutputWorkspace="engg_focus_input")
    van_integrated_ws = simple.Load(Filename=van_int)
    van_curves_ws = simple.Load(Filename=van_curves)
    for i in range(1, 3):
        output_ws = "engg_focus_output_bank_{}".format(i)
        simple.EnggFocus(InputWorkspace=ws_to_focus, OutputWorkspace=output_ws,
                         VanIntegrationWorkspace=van_integrated_ws, VanCurvesWorkspace=van_curves_ws,
                         Bank=str(i))
        _save_out(output_ws, run_number, str(i), focus_dir)
    print("done")


def focus_cropped(use_spectra, crop_on, van_curves, van_int, run_number, focus_dir):
    ws_to_focus = simple.Load(Filename="ENGINX" + run_number, OutputWorkspace="engg_focus_input")
    van_integrated_ws = simple.Load(Filename=van_int)
    van_curves_ws = simple.Load(Filename=van_curves)
    output_ws = "engg_focus_output{0}{1}"
    if not use_spectra:
        bank = {"North": "1",
                "South": "2"}
        output_ws = output_ws.format("_bank_", bank.get(crop_on))
        simple.EnggFocus(InputWorkspace=ws_to_focus, OutputWorkspace=output_ws,
                         VanIntegrationWorkspace=van_integrated_ws, VanCurvesWorkspace=van_curves_ws,
                         Bank=bank.get(crop_on))
        _save_out(output_ws, run_number, crop_on, focus_dir)
    else:
        output_ws = output_ws.format("", "")
        simple.EnggFocus(InputWorkspace=ws_to_focus, OutputWorkspace=output_ws,
                         VanIntegrationWorkspace=van_integrated_ws,
                         VanCurvesWorkspace=van_curves_ws, SpectrumNumbers=crop_on)
        _save_out(output_ws, run_number, "cropped", focus_dir)


def _save_out(output, run_number, bank_id, output_dir):
    if type(bank_id)is str:
        filename = os.path.join(output_dir, "ENGINX_{}_bank_{}".format(run_number, bank_id))
    else:
        filename = os.path.join(output_dir, "ENGINX_{}_{}".format(run_number, bank_id))
    simple.SaveFocusedXYE(InputWorkspace=output, Filename=filename, SplitFiles=False, StartAtBankNumber=bank_id)
    simple.SaveGSS(InputWorkspace=output, Filename=filename, SplitFiles=False, Bank=bank_id)
    simple.SaveOpenGenieAscii(InputWorkspace=output, Filename=filename, OpenGenieFormat="ENGIN-X Format")
    simple.SaveNexus(InputWorkspace=output, Filename=filename)
    simple.ExportSampleLogsToHDF5(InputWorkspace=output, Filename=filename, Blacklist="bankid")
