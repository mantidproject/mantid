# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import PolarizerEfficiency, Load
from SANSPolarizationCorrectionsBase import SANSPolarizationCorrectionsBase
from abc import abstractmethod, ABCMeta


class PolarizerEfficiencyTestBase(SANSPolarizationCorrectionsBase, metaclass=ABCMeta):
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

    @property
    @abstractmethod
    def input_efficiency_filename(self):
        """
        The filename to use as the input to the algorithm in this test.
        :return: The input workspace's filename.
        """
        pass

    def _run_test(self):
        pre_processed = self._prepare_workspace(self.input_filename)
        eff_name = f"{self.input_efficiency_filename}EfficiencyReference.nxs"
        eff = Load(eff_name)
        PolarizerEfficiency(pre_processed, eff, "00,10,11,01", OutputWorkspace="result")

    def _validate(self):
        result = "result"
        reference = f"{self.reference_basename}Reference.nxs"
        return result, reference


class PolarizerEfficiencyPolarisedTest(PolarizerEfficiencyTestBase):
    reference_basename = "PolarizerEfficiency"
    input_filename = "ZOOM00038249.nxs"
    input_efficiency_filename = "HeliumAnalyser"

    def __init__(self):
        PolarizerEfficiencyTestBase.__init__(self)


class PolarizerEfficiencyUnpolarisedTest(PolarizerEfficiencyTestBase):
    reference_basename = "UnpolPolarizerEfficiency"
    input_filename = "ZOOM00038253.nxs"
    input_efficiency_filename = "UnpolHeliumAnalyser"

    def __init__(self):
        PolarizerEfficiencyTestBase.__init__(self)
