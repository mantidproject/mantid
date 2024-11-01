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
import numpy as np
from mantid.simpleapi import HB3AAdjustSampleNorm, HB3AFindPeaks, HB3AIntegratePeaks, IntegratePeaksMD, mtd


class HB3AIntegratePeaksTest(unittest.TestCase):
    _files = "HB3A_exp0724_scan0182.nxs,HB3A_exp0724_scan0183.nxs"

    @classmethod
    def setUpClass(cls):
        # Create the workspaces needed for each test
        HB3AAdjustSampleNorm(Filename=cls._files, MergeInputs=True, OutputWorkspace="merged", NormaliseBy="None")
        HB3AFindPeaks(InputWorkspace=mtd["merged"], CellType="Orthorhombic", Centering="F", OutputWorkspace="peaks")

        IntegratePeaksMD(InputWorkspace=mtd["merged"], PeaksWorkspace=mtd["peaks"], PeakRadius=0.25, OutputWorkspace="int_peaksmd")

    @classmethod
    def tearDownClass(cls):
        mtd.clear()

    def test_integrate_peaks(self):
        # Test with no Lorentz correction - output should be identical to IntegratePeaksMD
        int_peaks = HB3AIntegratePeaks(InputWorkspace=mtd["merged"], PeaksWorkspace=mtd["peaks"], PeakRadius=0.25, ApplyLorentz=False)
        self.assertEqual(mtd["int_peaksmd"].getNumberPeaks(), int_peaks.getNumberPeaks())

        self.assertTrue(int_peaks.hasIntegratedPeaks())

        # Check that peak intensity matches those from IntegratePeaksMD
        for p in range(int_peaks.getNumberPeaks()):
            peak1 = mtd["int_peaksmd"].getPeak(p)
            peak2 = int_peaks.getPeak(p)

            self.assertAlmostEqual(peak1.getIntensity(), peak2.getIntensity())

        # test Lorentz correction, compare to previous integration
        int_peaks2 = HB3AIntegratePeaks(InputWorkspace=mtd["merged"], PeaksWorkspace=mtd["peaks"], PeakRadius=0.25)

        # Verify that both have the same number of peaks
        self.assertEqual(int_peaks2.getNumberPeaks(), int_peaks.getNumberPeaks())

        self.assertTrue(int_peaks2.hasIntegratedPeaks())

        # Check that peaks match - the sigma intensity shouldnt change even after lorentz
        for p in range(int_peaks2.getNumberPeaks()):
            peak1 = int_peaks.getPeak(p)
            peak2 = int_peaks2.getPeak(p)
            two_th = peak2.getScattering()
            az = peak2.getAzimuthal()
            nu = np.arcsin(np.sin(two_th) * np.sin(az))
            gamma = np.arctan(np.tan(two_th) * np.cos(az))
            # G. J. McIntyre and R. F. D. Stansfield, Acta Cryst A 44, 257 (1988).
            lorentz = abs(np.cos(nu) * np.sin(gamma))
            self.assertAlmostEqual(peak1.getIntensity() * lorentz, peak2.getIntensity())
            self.assertAlmostEqual(peak1.getSigmaIntensity() * lorentz, peak2.getSigmaIntensity())

    def test_integrate_peaks_output(self):
        test_dir = tempfile.mkdtemp()
        fileout = os.path.join(test_dir, "integrated_peaks_shelx")

        # Test SHELX output
        HB3AIntegratePeaks(
            InputWorkspace=mtd["merged"],
            PeaksWorkspace=mtd["peaks"],
            PeakRadius=0.25,
            OutputFormat="SHELX",
            OutputFile=fileout,
            OutputWorkspace="int_peaks",
        )
        self.assertTrue(os.path.exists(fileout))

        # Test Fullprof output
        fileout = os.path.join(test_dir, "integrated_peaks_fullprof")
        HB3AIntegratePeaks(
            InputWorkspace=mtd["merged"],
            PeaksWorkspace=mtd["peaks"],
            PeakRadius=0.25,
            OutputFormat="Fullprof",
            OutputFile=fileout,
            OutputWorkspace="int_peaks",
        )
        self.assertTrue(os.path.exists(fileout))

        shutil.rmtree(test_dir)


if __name__ == "__main__":
    unittest.main()
