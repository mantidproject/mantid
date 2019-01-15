# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as simple
import os

spec_nos = None
crop_name = "Cropped"
spec_num_used = False
van_run = "236516"
ceria_run = "241391"
van_name = "ENGINX" + ("0" * (8 - len(van_run))) + van_run
van_file_name = r"~/Work/Build-1/ExternalData/Testing/Data/UnitTest/" + van_name
curve_van = van_file_name + "_precalculated_vanadium_run_bank_curves.nxs"
int_van = van_file_name + "_precalculated_vanadium_run_integration.nxs"
van_curves_ws = simple.Load(curve_van, OutputWorkspace="curves_van")
van_integrated_ws = simple.Load(int_van, OutputWorkspace="int_van")
ceria_ws = simple.Load(Filename="ENGINX241391", OutputWorkspace="eng_calib")


def create_calibration_cropped_file(use_spectrum_number):
    if spec_num_used:
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


def create_calibration_files():
    banks = 3
    for i in range(1, banks):
        outFitParamsTblName = "engg_calibration_bank_" + str(i)
        print(outFitParamsTblName)
        simple.EnggCalibrate(InputWorkspace=ceria_ws, VanIntegrationWorkspace=van_integrated_ws,
                             VanCurvesWorkspace=van_curves_ws, Bank=str(i), FittedPeaks=outFitParamsTblName,
                             OutputParametersTableName=outFitParamsTblName)


print("Calibration done")
