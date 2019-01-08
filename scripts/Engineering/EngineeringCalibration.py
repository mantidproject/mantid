# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
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
banks = 0
if spec_num_used:
    banks = 2
    for i in range(1, banks):
        outFitParamsTblName = crop_name
        simple.EnggCalibrate(InputWorkspace=ceria_ws, VanIntegrationWorkspace=van_integrated_ws,
                             VanCurvesWorkspace=van_curves_ws, SpectrumNumbers=str(spec_nos),
                             FittedPeaks=outFitParamsTblName,
                             OutputParametersTableName=outFitParamsTblName)
elif spec_nos is not None:
    banks = 2
    for i in range(1, banks):
        outFitParamsTblName = ""
        bank = 0
        if spec_nos == "North":
            outFitParamsTblName = "engg_calibration_bank_1"
            bank = 1
        elif spec_nos == "South":
            outFitParamsTblName = "engg_calibration_bank_2"
            bank = 2
            simple.EnggCalibrate(InputWorkspace=ceria_ws, VanIntegrationWorkspace=van_integrated_ws,
                                 VanCurvesWorkspace=van_curves_ws, Bank=str(bank), FittedPeaks=outFitParamsTblName,
                                 OutputParametersTableName=outFitParamsTblName)
else:
    banks = 3
    for i in range(1, banks):
        outFitParamsTblName = "engg_calibration_bank_" + str(i)
        print(outFitParamsTblName)
        simple.EnggCalibrate(InputWorkspace=ceria_ws, VanIntegrationWorkspace=van_integrated_ws,
                             VanCurvesWorkspace=van_curves_ws, Bank=str(i), FittedPeaks=outFitParamsTblName,
                             OutputParametersTableName=outFitParamsTblName)
simple.SaveNexus(van_integrated_ws,
                 "/home/sjenkins/EnginX_Mantid/Calibration" + van_name + "_precalculated_vanadium_run_integration")
simple.SaveNexus(van_curves_ws,
                 "/home/sjenkins/EnginX_Mantid/Calibration" + van_name + "_precalculated_vanadium_run_bank_curves")
print("Calibration done")
