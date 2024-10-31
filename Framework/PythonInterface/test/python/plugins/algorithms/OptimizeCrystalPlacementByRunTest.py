# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import LoadIsawPeaks, FindUBUsingFFT, IndexPeaks, OptimizeCrystalPlacementByRun


class OptimizeCrystalPlacementByRunTest(unittest.TestCase):
    def test_simple(self):
        ws = LoadIsawPeaks("calibrated.peaks")
        FindUBUsingFFT(PeaksWorkspace=ws, MinD=2, MaxD=20, Tolerance=0.12)
        IndexPeaks(PeaksWorkspace="ws", Tolerance=0.12)
        wsd = OptimizeCrystalPlacementByRun(InputWorkspace=ws, Tolerance=0.12, StoreInADS=False)
        result = wsd.getPeak(0).getSamplePos()
        self.assertAlmostEqual(result.getX(), -0.000678629)
        self.assertAlmostEqual(result.getY(), -2.16033e-05)
        self.assertAlmostEqual(result.getZ(), 0.00493278)
        result = wsd.getPeak(8).getSamplePos()
        self.assertAlmostEqual(result.getX(), -0.0027929)
        self.assertAlmostEqual(result.getY(), -0.00105681)
        self.assertAlmostEqual(result.getZ(), 0.00497094)


if __name__ == "__main__":
    unittest.main()
