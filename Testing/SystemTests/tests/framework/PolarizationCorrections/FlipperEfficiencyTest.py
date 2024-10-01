# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import FlipperEfficiency
from SANSPolarizationCorrectionsBase import SANSPolarizationCorrectionsBase
from abc import abstractmethod, ABCMeta


class FlipperEfficiencyTestBase(SANSPolarizationCorrectionsBase, metaclass=ABCMeta):
    def __init__(self):
        SANSPolarizationCorrectionsBase.__init__(self)

    @property
    @abstractmethod
    def input_filename(self):
        """
        The filename to use as the input to the FlipperEfficiency algorithm in this test.
        :return: The input workspace's filename.
        """
        pass

    def _run_test(self):
        pre_processed = self._prepare_workspace(self.input_filename)
        FlipperEfficiency(pre_processed, "00,10,11,01", OutputWorkspace="result")

    def _validate(self):
        result = "result"
        reference = f"{self.reference_basename}Reference.nxs"
        return result, reference


class FlipperEfficiencyPolarisedTest(FlipperEfficiencyTestBase):
    reference_basename = "FlipperEfficiency"
    input_filename = "ZOOM00038249.nxs"

    def __init__(self):
        FlipperEfficiencyTestBase.__init__(self)
        self.nanEqual = True


class FlipperEfficiencyUnpolarisedTest(FlipperEfficiencyTestBase):
    reference_basename = "UnpolFlipperEfficiency"
    input_filename = "ZOOM00038253.nxs"

    def __init__(self):
        FlipperEfficiencyTestBase.__init__(self)
        self.nanEqual = True
