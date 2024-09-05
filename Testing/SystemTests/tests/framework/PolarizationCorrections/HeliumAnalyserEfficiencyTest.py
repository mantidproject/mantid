# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import *
from PolarizationCorrectionsBase import PolarizationCorrectionsBase


class HeliumAnalyserEfficiencyTest(PolarizationCorrectionsBase):
    def __init__(self):
        PolarizationCorrectionsBase.__init__(self)

    def _run_test(self):
        pass

    def _validate(self):
        pass
