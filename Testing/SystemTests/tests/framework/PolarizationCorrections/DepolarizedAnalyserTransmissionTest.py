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
        Load("ZOOM00038238.nxs", OutputWorkspace="mt_run")
        Load("ZOOM00038335.nxs", OutputWorkspace="dep_run")

        mt_group = self._prepare_workspace("mt_run")
        dep_group = self._prepare_workspace("dep_run")
        mt = self._average_workspaces_in_group(list(mt_group))
        dep = self._average_workspaces_in_group(list(dep_group))

        DepolarizedAnalyserTransmission(dep, mt, OutputWorkspace="params" "", OutputFitCurves="curves")

    def _prepare_workspace(self, input_ws_name):
        converted = ConvertUnits(input_ws_name, "Wavelength", AlignBins=True, StoreInADS=False)
        monitor_3 = CropWorkspace(converted, StartWorkspaceIndex=2, EndWorkspaceIndex=2, StoreInADS=False)
        monitor_4 = CropWorkspace(converted, StartWorkspaceIndex=3, EndWorkspaceIndex=3, StoreInADS=False)
        return monitor_4 / monitor_3

    def _average_workspaces_in_group(self, ws_list):
        return sum(ws_list) / 4

    def validate(self):
        self.tolerance = 1e-5
        result_curves = "curves"
        reference_curves = "DepolCurvesReference.nxs"
        result_params = "params"
        reference_params = "DepolParamsReference.nxs"

        def validate_group(result, reference):
            Load(Filename=reference, OutputWorkspace=reference)
            compare_alg = AlgorithmManager.create("CompareWorkspaces")
            compare_alg.setPropertyValue("Workspace1", result)
            compare_alg.setPropertyValue("Workspace2", reference)
            compare_alg.setPropertyValue("Tolerance", str(self.tolerance))
            compare_alg.setChild(True)

            compare_alg.execute()
            if compare_alg.getPropertyValue("Result") != "1":
                print("Workspaces do not match.")
                print(self.__class__.__name__)
                SaveNexus(InputWorkspace=result, Filename=f"{self.__class__.__name__}-{result}-mismatch.nxs")
                return False
            return True

        is_curves_match = validate_group(result_curves, reference_curves)
        is_params_match = validate_group(result_params, reference_params)
        return is_curves_match and is_params_match

    def cleanup(self):
        AnalysisDataService.clear()
