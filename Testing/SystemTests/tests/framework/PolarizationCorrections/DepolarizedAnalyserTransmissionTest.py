# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting

from mantid.api import AnalysisDataService
from mantid.simpleapi import *


class DepolarizedAnalyzerTransmissionTest(systemtesting.MantidSystemTest):
    def runTest(self):
        Load("/Users/caila.finn/MantidData/PolSANS/DepolAnalyser/ZOOM00038238.nxs", OutputWorkspace="mt_run")
        Load("/Users/caila.finn/MantidData/PolSANS/DepolAnalyser/ZOOM00038335.nxs", OutputWorkspace="dep_run")

        self._prepare_workspace("mt_run", "mt")
        self._prepare_workspace("dep_run", "dep_group")
        self._average_workspaces_in_group("dep_group", "dep")

        DepolarizedAnalyserTransmission("dep", "mt", OutputWorkspace="params", OutputFitCurves="curves")

    def _prepare_workspace(self, input_ws_name, output_ws_name):
        ConvertUnits(input_ws_name, "Wavelength", AlignBins=True, OutputWorkspace="__temp_wl")
        CropWorkspace("__temp_wl", StartWorkspaceIndex=2, EndWorkspaceIndex=2, OutputWorkspace="__temp_mon3")
        CropWorkspace("__temp_wl", StartWorkspaceIndex=3, EndWorkspaceIndex=3, OutputWorkspace="__temp_mon4")
        Divide(LHSWorkspace="__temp_mon4", RHSWorkspace="__temp_mon3", OutputWorkspace=output_ws_name)

    def _average_workspaces_in_group(self, input_group_name, output_name):
        group = AnalysisDataService.retrieve(input_group_name)
        summed = group.getItem(0)
        for i in range(1, 3):
            summed = summed + group.getItem(i)
        AnalysisDataService.addOrReplace(output_name, summed / 4)

    def validate(self):
        pass
