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

        DepolarizedAnalyserTransmission("dep", "mt", OutputWorkspace="params", OutputFitCurves="curves", IgnoreFitQualityError=True)

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
        self.tolerance = 1e-5
        result_curves = "curves"
        reference_curves = "/Users/caila.finn/MantidData/PolSANS/DepolAnalyser/DepolCurvesReference.nxs"
        result_params = "params"
        reference_params = "/Users/caila.finn/MantidData/PolSANS/DepolAnalyser/DepolParamsReference.nxs"

        def validate_group(result, reference):
            value_names = [result, reference]

            Load(Filename=reference, OutputWorkspace=reference)
            compare_alg = AlgorithmManager.create("CompareWorkspaces")
            compare_alg.setPropertyValue("Workspace1", value_names[0])
            compare_alg.setPropertyValue("Workspace2", value_names[1])
            compare_alg.setPropertyValue("Tolerance", str(self.tolerance))
            compare_alg.setChild(True)

            compare_alg.execute()
            if compare_alg.getPropertyValue("Result") != "1":
                print(" Workspaces do not match. Result: ", compare_alg.getPropertyValue("Result"))
                print(self.__class__.__name__)
                SaveNexus(InputWorkspace=value_names[0], Filename=self.__class__.__name__ + "-mismatch.nxs")
                return False
            return True

        return validate_group(result_curves, reference_curves) and validate_group(result_params, reference_params)
