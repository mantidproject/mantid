# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as simple


def create_vanadium_workspaces(vanadium_run, curve_van, int_van):
    van_name = "eng_vanadium_ws"
    simple.Load(vanadium_run, OutputWorkspace=van_name)
    simple.EnggVanadiumCorrections(VanadiumWorkspace=van_name,
                                   OutIntegrationWorkspace="eng_vanadium_integration",
                                   OutCurvesWorkspace="eng_vanadium_curves")
    simple.SaveNexus("eng_vanadium_integration", int_van)
    simple.SaveNexus("eng_vanadium_curves", curve_van)
    simple.DeleteWorkspace(van_name)


def create_calibration_cropped_file(use_spectrum_number, spec_nos, crop_name, curve_van, int_van, ceria_run):
    van_curves_ws, van_integrated_ws = load_van_files(curve_van, int_van)
    ceria_ws = simple.Load(Filename="ENGINX"+ceria_run, OutputWorkspace="eng_calib")
    if use_spectrum_number:
        if crop_name is None:
            param_tbl_name = "cropped"
        else:
            param_tbl_name = crop_name
        simple.EnggCalibrate(InputWorkspace=ceria_ws, VanIntegrationWorkspace=van_integrated_ws,
                             VanCurvesWorkspace=van_curves_ws, SpectrumNumbers=str(spec_nos),
                             FittedPeaks=param_tbl_name,
                             OutputParametersTableName=param_tbl_name)
    else:
        if spec_nos == "North":
            param_tbl_name = "engg_calibration_bank_1"
            bank = 1
        elif spec_nos == "South":
            param_tbl_name = "engg_calibration_bank_2"
            bank = 2
        simple.EnggCalibrate(InputWorkspace=ceria_ws, VanIntegrationWorkspace=van_integrated_ws,
                             VanCurvesWorkspace=van_curves_ws, Bank=str(bank), FittedPeaks=param_tbl_name,
                             OutputParametersTableName=param_tbl_name)


def create_calibration_files(curve_van, int_van, ceria_run):
    van_curves_ws, van_integrated_ws = load_van_files(curve_van, int_van)
    ceria_ws = simple.Load(Filename="ENGINX"+ceria_run, OutputWorkspace="eng_calib")
    banks = 3
    for i in range(1, banks):
        param_tbl_name = "engg_calibration_bank_{}".format(i)
        print(param_tbl_name)
        simple.EnggCalibrate(InputWorkspace=ceria_ws, VanIntegrationWorkspace=van_integrated_ws,
                             VanCurvesWorkspace=van_curves_ws, Bank=str(i), FittedPeaks=param_tbl_name,
                             OutputParametersTableName=param_tbl_name)


def load_van_files(curves_van, ints_van):
    van_curves_ws = simple.Load(curves_van, OutputWorkspace="curves_van")
    van_integrated_ws = simple.Load(ints_van, OutputWorkspace="int_van")
    return van_curves_ws, van_integrated_ws

