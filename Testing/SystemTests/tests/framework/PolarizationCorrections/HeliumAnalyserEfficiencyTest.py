# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from abc import ABCMeta, abstractmethod
from mantid.simpleapi import HeliumAnalyserEfficiency
from SANSPolarizationCorrectionsBase import SANSPolarizationCorrectionsBase


class HeliumAnalyserEfficiencyTestBase(SANSPolarizationCorrectionsBase, metaclass=ABCMeta):
    def __init__(self):
        SANSPolarizationCorrectionsBase.__init__(self)

    @property
    @abstractmethod
    def input_filename(self):
        """
        The filename to use as the input to the algorithm in this test.
        :return: The input workspace's filename.
        """
        pass

    def _run_test(self):
        pre_processed = self._prepare_workspace(self.input_filename)

        HeliumAnalyserEfficiency(
            pre_processed, "00,10,11,01", OutputWorkspace="efficiency", OutputFitCurves="curves", OutputFitParameters="params"
        )

    def _validate(self):
        result_eff = "efficiency"
        reference_eff = f"{self.reference_basename}EfficiencyReference.nxs"
        result_curves = "curves"
        reference_curves = f"{self.reference_basename}CurvesReference.nxs"
        result_params = "params"
        reference_params = f"{self.reference_basename}ParamsReference.nxs"

        return result_eff, reference_eff, result_curves, reference_curves, result_params, reference_params


class HeliumAnalyserEfficiencyPolarisedTest(HeliumAnalyserEfficiencyTestBase):
    reference_basename = "HeliumAnalyser"
    input_filename = "ZOOM00038249.nxs"

    def __init__(self):
        HeliumAnalyserEfficiencyTestBase.__init__(self)


class HeliumAnalyserEfficiencyUnpolarisedTest(HeliumAnalyserEfficiencyTestBase):
    reference_basename = "UnpolHeliumAnalyser"
    input_filename = "ZOOM00038253.nxs"

    def __init__(self):
        HeliumAnalyserEfficiencyTestBase.__init__(self)
