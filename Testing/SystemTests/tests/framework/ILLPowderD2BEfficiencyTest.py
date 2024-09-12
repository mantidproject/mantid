# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import PowderILLEfficiency, GroupWorkspaces
from mantid import config, mtd
import numpy as np


class ILLPowderD2BEfficiencyTest(systemtesting.MantidSystemTest):
    def __init__(self):
        super(ILLPowderD2BEfficiencyTest, self).__init__()
        self.setUp()

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D2B"
        config.appendDataSearchSubDir("ILL/D2B/")

    def requiredFiles(self):
        return ["532008.nxs", "532009.nxs"]

    def cleanup(self):
        mtd.clear()

    def testAutoMasking(self):
        PowderILLEfficiency(
            CalibrationRun="532008,532009",
            DerivationMethod="GlobalSummedReference2D",
            ExcludedRange=[-5, 10],
            OutputWorkspace="masked",
            MaskCriterion=[0.3, 3],
        )
        data = mtd["masked"].extractY().flatten()
        data = data[np.nonzero(data)]
        coeff_max = data.max()
        self.assertLessEqual(coeff_max, 3.0)
        coeff_min = data.min()
        self.assertGreaterEqual(coeff_min, 0.3)

    def runTest(self):
        self.testAutoMasking()

        PowderILLEfficiency(
            CalibrationRun="532008,532009",
            DerivationMethod="GlobalSummedReference2D",
            ExcludedRange=[-5, 10],
            OutputWorkspace="calib",
            OutputResponseWorkspace="response",
        )
        GroupWorkspaces(InputWorkspaces=["calib", "response"], OutputWorkspace="group")

    def validate(self):
        self.tolerance = 0.01
        return ["group", "D2B_DetEffCorr_Ref.nxs"]
