from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np
from mantid.simpleapi import ApplyPowderDiffILLDetEffCorr, CreateWorkspace, CreateSampleWorkspace


class ApplyPowderDiffILLDetEffCorrTest(unittest.TestCase):
    def test_standard_case(self):
        input_ws = CreateSampleWorkspace(NumMonitors=1, NumBanks=6, BankPixelWidth=1, XMin=0, XMax=1, BinWidth=1)

        calibration_x = np.array([0, 0, 0, 0, 0, 0])
        calibration_y = np.array([1.0, 2.0, 3.0, 4.0, 5.0, 6.0])
        calibration_ws = CreateWorkspace(DataX=calibration_x, DataY=calibration_y, Nspec=calibration_y.size)

        calibrated_ws = ApplyPowderDiffILLDetEffCorr(input_ws, calibration_ws)
        for i in range(1, 7):
            self.assertEquals(calibrated_ws.readY(i), input_ws.readY(i) * i)
            self.assertEquals(calibrated_ws.readE(i), input_ws.readE(i) * i)

    def test_mismatched_workspace_size(self):
        input_ws = CreateSampleWorkspace(NumMonitors=1, NumBanks=6, BankPixelWidth=1, XMin=0, XMax=1, BinWidth=1)

        calibration_x = np.array([0, 0, 0, 0, 0, 0])
        calibration_y = np.array([1.0, 2.0, 3.0, 4.0, 5.0, 6.0])
        calibration_ws = CreateWorkspace(DataX=calibration_x, DataY=calibration_y, Nspec=calibration_y.size)

        self.assertRaises(ValueError, ApplyPowderDiffILLDetEffCorr, InputWorkspace=input_ws,
                          DetectorEfficiencyWorkspace=calibration_ws, OutputWorkspace='')

if __name__ == "__main__":
    unittest.main()
