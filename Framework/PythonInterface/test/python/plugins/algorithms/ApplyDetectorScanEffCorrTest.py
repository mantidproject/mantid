# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from mantid.api import mtd
from mantid.simpleapi import ApplyDetectorScanEffCorr, CreateSampleWorkspace, CreateWorkspace, ExtractMonitors, LoadILLDiffraction, LoadNexusProcessed, MaskBinsIf, Transpose


class ApplyDetectorScanEffCorrTest(unittest.TestCase):
    def test_non_scanning_case(self):
        input_ws = CreateSampleWorkspace(NumMonitors=0, NumBanks=6, BankPixelWidth=1, XMin=0, XMax=1, BinWidth=1)

        calibration_x = np.array([0, 0, 0, 0, 0, 0])
        calibration_y = np.array([1.0, 2.0, 3.0, 4.0, 5.0, 6.0])

        calibration_ws = CreateWorkspace(DataX=calibration_x, DataY=calibration_y, Nspec=calibration_y.size)

        calibrated_ws = ApplyDetectorScanEffCorr(input_ws, calibration_ws)
        for i in range(6):
            self.assertEqual(calibrated_ws.readY(i), input_ws.readY(i) * (i + 1))
            self.assertEqual(calibrated_ws.readE(i), input_ws.readE(i) * (i + 1))

    def test_simple_scanning_case(self):
        input_ws = CreateSampleWorkspace(NumMonitors=0, NumBanks=6, BankPixelWidth=1, XMin=0, XMax=1, BinWidth=1, NumScanPoints=2)

        calibration_x = np.array([0, 0, 0, 0, 0, 0])
        calibration_y = np.array([1.0, 2.0, 3.0, 4.0, 5.0, 6.0])

        # Note the monitors are in the wrong place doing the test workspace creation like this - but it does not affect the test.
        calibration_ws = CreateWorkspace(DataX=calibration_x, DataY=calibration_y, Nspec=calibration_y.size)

        expected = np.repeat(calibration_y, 2)
        calibrated_ws = ApplyDetectorScanEffCorr(input_ws, calibration_ws)
        for i in range(12):
            self.assertEqual(calibrated_ws.readY(i), input_ws.readY(i) * expected[i])
            self.assertEqual(calibrated_ws.readE(i), input_ws.readE(i) * expected[i])

    def test_mismatched_workspace_size(self):
        input_ws = CreateSampleWorkspace(NumMonitors=0, NumBanks=7, BankPixelWidth=1, XMin=0, XMax=1, BinWidth=1)

        calibration_x = np.array([0, 0, 0, 0, 0, 0])
        calibration_y = np.array([1.0, 2.0, 3.0, 4.0, 5.0, 6.0])
        calibration_ws = CreateWorkspace(DataX=calibration_x, DataY=calibration_y, Nspec=calibration_y.size)

        self.assertRaisesRegex(
            RuntimeError,
            "Number of histograms in input workspace is not a multiple of number of entries in detector efficiency workspace.",
            ApplyDetectorScanEffCorr,
            InputWorkspace=input_ws,
            DetectorEfficiencyWorkspace=calibration_ws,
            OutputWorkspace="test",
        )

    def test_2d_scanning_workspace(self):
        input_ws = CreateSampleWorkspace(NumMonitors=0, NumBanks=3, BankPixelWidth=2, XMin=0, XMax=5, BinWidth=1, NumScanPoints=7)

        calibration_x = np.array([0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2])
        calibration_y = np.arange(12)
        calibration_ws = CreateWorkspace(DataX=calibration_x, DataY=calibration_y, Nspec=4)
        calibrated_ws = ApplyDetectorScanEffCorr(input_ws, calibration_ws)

        tmp = Transpose(calibration_ws)
        tmp = tmp.extractY().flatten()
        to_multiply = np.repeat(tmp, 5 * 7)
        to_multiply = np.reshape(to_multiply, [7 * 12, 5])

        for det in range(7 * 12):
            for bin in range(5):
                self.assertEqual(calibrated_ws.readY(det)[bin], input_ws.readY(det)[bin] * to_multiply[det][bin])

    def test_masking(self):
        LoadILLDiffraction(Filename="ILL/D2B/508093.nxs", OutputWorkspace="scan")
        ExtractMonitors(InputWorkspace="scan", DetectorWorkspace="scan")
        LoadNexusProcessed(Filename="ILL/D2B/test_calib.nxs", OutputWorkspace="calib")
        # this will mask tube #9 pixel #127
        MaskBinsIf(InputWorkspace="calib", OutputWorkspace="calib", Criterion="y>9")
        ApplyDetectorScanEffCorr(InputWorkspace="scan", DetectorEfficiencyWorkspace="calib", OutputWorkspace="scan")
        spec = 8 * 128 * 25 + 126 * 25
        specInfo = mtd["scan"].spectrumInfo()
        for wsIndex in range(spec, spec + 25):
            self.assertTrue(specInfo.isMasked(spec))


if __name__ == "__main__":
    unittest.main()
