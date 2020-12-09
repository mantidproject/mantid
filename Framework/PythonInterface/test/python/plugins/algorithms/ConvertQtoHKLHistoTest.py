# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import BinMD, ConvertToMD, ConvertQtoHKLHisto, HB3AAdjustSampleNorm, HB3AFindPeaks, Load, \
    LoadIsawUB, mtd, SetGoniometer
import numpy as np


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

    def test_qtohkl_corelli(self):
        Load(Filename='CORELLI_29782.nxs', OutputWorkspace='data')
        SetGoniometer(Workspace='data', Axis0='BL9:Mot:Sample:Axis1,0,1,0,1')
        LoadIsawUB(InputWorkspace='data', Filename='SingleCrystalDiffuseReduction_UB.mat')
        ConvertToMD(InputWorkspace='data',
                    QDimensions='Q3D',
                    dEAnalysisMode='Elastic',
                    Q3DFrames='HKL',
                    QConversionScales='HKL',
                    OutputWorkspace='HKL',
                    Uproj='1,1,0',
                    Vproj='1,-1,0',
                    Wproj='0,0,1',
                    MinValues='-10,-10,-10',
                    MaxValues='10,10,10')
        HKL_binned = BinMD('HKL',
                           AlignedDim0='[H,H,0],-10.05,10.05,201',
                           AlignedDim1='[H,-H,0],-10.05,10.05,201',
                           AlignedDim2='[0,0,L],-1.05,1.05,21')
        ConvertToMD(InputWorkspace='data',
                    QDimensions='Q3D',
                    dEAnalysisMode='Elastic',
                    Q3DFrames='Q_sample',
                    OutputWorkspace='q_sample',
                    MinValues='-10,-10,-10',
                    MaxValues='10,10,10')

        hkl = ConvertQtoHKLHisto(InputWorkspace=mtd["q_sample"],
                                 FindUBFromPeaks=False,
                                 Extents="-10.05,10.05,-10.05,10.05,-1.05,1.05",
                                 Bins="201,201,21")

        orig_sig = HKL_binned.getSignalArray()
        new_sig = hkl.getSignalArray()

        np.testing.assert_allclose(np.asarray(new_sig), np.asarray(orig_sig))


if __name__ == '__main__':
    unittest.main()
