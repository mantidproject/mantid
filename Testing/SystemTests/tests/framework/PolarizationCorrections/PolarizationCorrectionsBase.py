# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from abc import ABCMeta, abstractmethod
from systemtesting import MantidSystemTest


from mantid.api import AnalysisDataService
from mantid.simpleapi import *


class PolarizationCorrectionsBase(MantidSystemTest, metaclass=ABCMeta):
    def runTest(self):
        self._run_test()

    @abstractmethod
    def _run_test(self):
        raise NotImplementedError("_run_test() method must be implemented.")

    def _prepare_workspace(self, input_ws_name):
        converted = ConvertUnits(input_ws_name, "Wavelength", AlignBins=True, StoreInADS=False)
        monitor_3 = CropWorkspace(converted, StartWorkspaceIndex=2, EndWorkspaceIndex=2, StoreInADS=False)
        monitor_4 = CropWorkspace(converted, StartWorkspaceIndex=3, EndWorkspaceIndex=3, StoreInADS=False)
        return monitor_4 / monitor_3

    def _average_workspaces_in_group(self, ws_list):
        return sum(ws_list) / len(ws_list)

    def _validate_workspace(self, result, reference):
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

    def validate(self):
        return self._validate()

    @abstractmethod
    def _validate(self):
        raise NotImplementedError("validate() method must be implemented.")

    def cleanup(self):
        AnalysisDataService.clear()
