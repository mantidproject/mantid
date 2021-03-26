# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import tempfile
import shutil
import os
from mantid.simpleapi import HB3AAdjustSampleNorm, HB3AFindPeaks, HB3AIntegratePeaks, IntegratePeaksMD, mtd


class HB3AIntegratePeaksTest(unittest.TestCase):

    _files = "HB3A_exp0724_scan0182.nxs,HB3A_exp0724_scan0183.nxs"

    @classmethod
    def setUpClass(cls):
        # Create the workspaces needed for each test
        HB3AAdjustSampleNorm(Filename=cls._files, MergeInputs=True, OutputWorkspace="merged")
        HB3AFindPeaks(InputWorkspace=mtd["merged"], CellType="Orthorhombic", Centering="F", OutputWorkspace="peaks")

        IntegratePeaksMD(InputWorkspace=mtd["merged"],
                         PeaksWorkspace=mtd["peaks"],
                         PeakRadius=0.25,
                         OutputWorkspace="int_peaksmd")

    @classmethod
    def tearDownClass(cls):
        mtd.clear()

    def test_integrate_peaks(self):
        # Test with no Lorentz correction - output should be identical to IntegratePeaksMD
        int_peaks = HB3AIntegratePeaks(InputWorkspace=mtd["merged"],
                                       PeaksWorkspace=mtd["peaks"],
                                       PeakRadius=0.25,
                                       ApplyLorentz=False)
        self.assertEqual(mtd['int_peaksmd'].getNumberPeaks(), int_peaks.getNumberPeaks())

        self.assertTrue(int_peaks.hasIntegratedPeaks())

        # Check that peak intensity matches those from IntegratePeaksMD
        for p in range(int_peaks.getNumberPeaks()):
            peak1 = mtd['int_peaksmd'].getPeak(p)
            peak2 = int_peaks.getPeak(p)

            self.assertAlmostEqual(peak1.getIntensity(), peak2.getIntensity())

    def test_integrate_peaks_lorentz(self):
        # Make sure that this algorithm gives similar results to IntegratePeaksMD
        int_peaks = HB3AIntegratePeaks(InputWorkspace=mtd["merged"], PeaksWorkspace=mtd["peaks"], PeakRadius=0.25)

        # Verify that both have the same number of peaks
        self.assertEqual(mtd['int_peaksmd'].getNumberPeaks(), int_peaks.getNumberPeaks())

        self.assertTrue(int_peaks.hasIntegratedPeaks())

        # Check that peaks match - the sigma intensity shouldnt change even after lorentz
        for p in range(int_peaks.getNumberPeaks()):
            peak1 = mtd['int_peaksmd'].getPeak(p)
            peak2 = int_peaks.getPeak(p)

            self.assertAlmostEqual(peak1.getSigmaIntensity(), peak2.getSigmaIntensity())

    def test_integrate_peaks_output(self):
        test_dir = tempfile.mkdtemp()
        fileout = os.path.join(test_dir, "integrated_peaks_shelx")

        # Test SHELX output
        HB3AIntegratePeaks(InputWorkspace=mtd["merged"],
                           PeaksWorkspace=mtd["peaks"],
                           PeakRadius=0.25,
                           OutputFormat="SHELX",
                           OutputFile=fileout,
                           OutputWorkspace="int_peaks")
        self.assertTrue(os.path.exists(fileout))

        # Test Fullprof output
        fileout = os.path.join(test_dir, "integrated_peaks_fullprof")
        HB3AIntegratePeaks(InputWorkspace=mtd["merged"],
                           PeaksWorkspace=mtd["peaks"],
                           PeakRadius=0.25,
                           OutputFormat="Fullprof",
                           OutputFile=fileout,
                           OutputWorkspace="int_peaks")
        self.assertTrue(os.path.exists(fileout))

        shutil.rmtree(test_dir)


if __name__ == '__main__':
    unittest.main()
