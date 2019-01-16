# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as simple
from shutil import copy2
from os import path
import csv


def focus_whole(van_curves, van_int, run_number, focus_dir, focus_gen):
    ws_to_focus = simple.Load(Filename="ENGINX" + run_number, OutputWorkspace="engg_focus_input")
    van_integrated_ws = simple.Load(Filename=van_int)
    van_curves_ws = simple.Load(Filename=van_curves)
    for i in range(1, 3):
        output_ws = "engg_focus_output_bank_{}".format(i)
        simple.EnggFocus(InputWorkspace=ws_to_focus, OutputWorkspace=output_ws,
                         VanIntegrationWorkspace=van_integrated_ws, VanCurvesWorkspace=van_curves_ws,
                         Bank=str(i))
        _save_out(output_ws, run_number, str(i), focus_dir, "ENGINX_{}_{}", focus_gen)
    print("done")


def focus_cropped(use_spectra, crop_on, van_curves, van_int, run_number, focus_dir, focus_gen):
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
        _save_out(output_ws, run_number, crop_on, focus_dir, "ENGINX_{}_{}", focus_gen)
    else:
        output_ws = output_ws.format("", "")
        simple.EnggFocus(InputWorkspace=ws_to_focus, OutputWorkspace=output_ws,
                         VanIntegrationWorkspace=van_integrated_ws,
                         VanCurvesWorkspace=van_curves_ws, SpectrumNumbers=crop_on)
        _save_out(output_ws, run_number, "cropped", focus_dir, "ENGINX_{}_bank_{}", focus_gen)


def focus_texture_mode(van_curves, van_int, run_number, focus_dir, dg_file, focus_gen):
    van_curves_ws, van_integrated_ws, ws_to_focus = _prepare_focus(run_number, van_curves, van_int)
    banks = {}
    with open(dg_file) as grouping_file:
        group_reader = csv.reader(_decomment_csv(grouping_file), delimiter=',')

        for row in group_reader:
            banks.update({row[0]: ','.join(row[1:])})

    for bank in banks:
        output_ws = "engg_focusing_output_ws_texture_bank_{}"
        output_ws = output_ws.format(bank)
        print(banks.get(bank))
        simple.EnggFocus(InputWorkspace=ws_to_focus, OutputWorkspace=output_ws,
                         VanIntegrationWorkspace=van_integrated_ws, VanCurvesWorkspace=van_curves_ws,
                         SpectrumNumbers=banks.get(bank))
        _save_out(output_ws, run_number, bank, focus_dir, "ENGINX_{}_texture_{}", focus_gen)


def _prepare_focus(run_number, van_curves, van_int):
    ws_to_focus = simple.Load(Filename="ENGINX" + run_number, OutputWorkspace="engg_focus_input")
    van_integrated_ws = simple.Load(Filename=van_int)
    van_curves_ws = simple.Load(Filename=van_curves)
    return van_curves_ws, van_integrated_ws, ws_to_focus


def _save_out(output, run_number, bank_id, output_dir, join_string, focus_gen):
    filename = path.join(output_dir, join_string.format(run_number, bank_id))
    hdf5_name = path.join(output_dir, run_number + ".hdf5")
    simple.SaveFocusedXYE(InputWorkspace=output, Filename=filename + ".dat", SplitFiles=False,
                          StartAtBankNumber=bank_id)
    simple.SaveGSS(InputWorkspace=output, Filename=filename + ".gss", SplitFiles=False, Bank=bank_id)
    simple.SaveOpenGenieAscii(InputWorkspace=output, Filename=filename + ".his", OpenGenieFormat="ENGIN-X Format")
    simple.SaveNexus(InputWorkspace=output, Filename=filename + ".nxs")
    simple.ExportSampleLogsToHDF5(InputWorkspace=output, Filename=hdf5_name, Blacklist="bankid")
    copy2(filename+".dat", focus_gen)
    copy2(filename + ".gss", focus_gen)
    copy2(filename + ".his", focus_gen)
    copy2(filename + ".nxs", focus_gen)
    copy2(hdf5_name, focus_gen)


def _decomment_csv(csvfile):
    for row in csvfile:
        raw = row.split('#')[0].strip()
        if raw:
            yield raw
