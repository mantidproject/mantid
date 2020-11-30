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

    def test_integrate_peaks(self):
        # Make sure that this algorithm gives similar results to IntegratePeaksMD

        HB3AAdjustSampleNorm(Filename=self._files, MergeInputs=True, OutputWorkspace="merged")
        HB3AFindPeaks(InputWorkspace=mtd["merged"],
                      CellType="Orthorhombic",
                      Centering="F",
                      OutputWorkspace="peaks")

        int_peaks = IntegratePeaksMD(InputWorkspace=mtd["merged"], PeaksWorkspace=mtd["peaks"], PeakRadius=0.25)

        test_dir = tempfile.mkdtemp()
        fileout = os.path.join(test_dir, "integrated_peaks.hkl")

        corrected_peaks = HB3AIntegratePeaks(InputWorkspace=mtd["merged"], PeaksWorkspace=mtd["peaks"],
                                             PeakRadius=0.25, OutputFile=fileout)

        # Verify that both have the same number of peaks
        self.assertEqual(int_peaks.getNumberPeaks(), corrected_peaks.getNumberPeaks())

        # Check that peaks match
        for p in range(corrected_peaks.getNumberPeaks()):
            peak1 = int_peaks.getPeak(p)
            peak2 = corrected_peaks.getPeak(p)

            self.assertAlmostEqual(peak1.getSigmaIntensity(), peak2.getSigmaIntensity())

        mtd.clear()
        shutil.rmtree(test_dir)


if __name__ == '__main__':
    unittest.main()
