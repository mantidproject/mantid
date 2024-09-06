# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import FlipperEfficiency
from SANSPolarizationCorrectionsBase import SANSPolarizationCorrectionsBase


class FlipperEfficiencyTest(SANSPolarizationCorrectionsBase):
    def __init__(self):
        SANSPolarizationCorrectionsBase.__init__(self)

    def _run_test(self):
        pre_processed = self._prepare_workspace("ZOOM00038249.nxs")
        FlipperEfficiency(pre_processed, "00,10,11,01", OutputWorkspace="result")

    def _validate(self):
        result = "result"
        reference = "/Users/caila.finn/MantidData/PolSANS/FlipperEfficiency/FlipperEfficiencyReference.nxs"
        return result, reference
