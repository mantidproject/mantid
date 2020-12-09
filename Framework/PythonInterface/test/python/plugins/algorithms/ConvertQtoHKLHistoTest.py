# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import HB3AAdjustSampleNorm, HB3AFindPeaks, ConvertQtoHKLHisto, mtd


class ConvertQtoHKLHistoTest(unittest.TestCase):

    _files = "HB3A_exp0724_scan0182.nxs"

    @classmethod
    def setUpClass(cls):
        HB3AAdjustSampleNorm(Filename=cls._files, OutputWorkspace="mde_ws")

        HB3AFindPeaks(InputWorkspace=mtd["mde_ws"],
                      CellType="Tetragonal",
                      Centering="I",
                      PeakDistanceThreshold=0.25,
                      DensityThresholdFactor=5000,
                      Wavelength=1.558,
                      OutputWorkspace="peaks")

    @classmethod
    def tearDownClass(cls):
        mtd.clear()

    def test_qtohkl(self):
        # Tests the conversion from the MDEvent WS in Q Sample to a MDHisto WS in HKL using this algorithm.
        hkl = ConvertQtoHKLHisto(InputWorkspace=mtd["mde_ws"], FindUBFromPeaks=False)
        self.assertEqual(hkl.getSpecialCoordinateSystem().name, "HKL")

        hkl = ConvertQtoHKLHisto(InputWorkspace=mtd["mde_ws"],
                                 FindUBFromPeaks=True,
                                 PeaksWorkspace=mtd["peaks"])
        self.assertEqual(hkl.getSpecialCoordinateSystem().name, "HKL")

        self.assertEqual(mtd["peaks"].getNumberPeaks(), 12)
        peak = mtd["peaks"].getPeak(0)
        self.assertAlmostEqual(peak.getH(), -0.995644, delta=1.0e-5)
        self.assertAlmostEqual(peak.getK(), 0.977317, delta=1.0e-5)
        self.assertAlmostEqual(peak.getL(), 3.93929, delta=1.0e-5)


if __name__ == '__main__':
    unittest.main()
