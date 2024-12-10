# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import LRPeakSelection, CreateSampleWorkspace, DeleteWorkspace


class LRPeakSelectionTest(unittest.TestCase):
    def setUp(self):
        self.ws = None

    def tearDown(self):
        if self.ws is not None:
            DeleteWorkspace(self.ws)

    def _workspace(self, userFun):
        if self.ws is not None:
            DeleteWorkspace(self.ws)

        self.ws = CreateSampleWorkspace(
            OutputWorkspace="out",
            Function="User Defined",
            UserDefinedFunction=userFun,
            NumBanks=1,
            BankPixelWidth=1,
            XMin=0,
            XMax=300,
            BinWidth=1.0,
        )

    def _gaussianWorkspace(self, peakCentre, height, sigma):
        self._workspace("name=Gaussian,PeakCentre=%s,Height=%s,Sigma=%s" % (peakCentre, height, sigma))

    def test_peaks(self):
        """
        Test that a reflectivity peak is found to match a generated Gaussian.
        """
        peak_center = 150.0
        sigma = 25.0
        self._gaussianWorkspace(peak_center, 1000, sigma)
        peak, low_res, _ = LRPeakSelection(self.ws)

        # Require a close relative match between given and fitted values.
        # The width of the fitted peak should be about twice the sigma.
        position = (peak[0] + peak[1]) / 2.0
        width = abs(peak[1] - peak[0]) / 4.0
        diff_peak_centre = abs((position - peak_center) / peak_center)
        diff_sigma = abs((width - sigma) / sigma)

        self.assertLess(diff_peak_centre, 0.03)
        self.assertLess(diff_sigma, 0.05)

    def test_primary_range(self):
        """
        Test that a reflectivity peak is found to match a generated Gaussian,
        with the primary range turned on.
        """
        peak_center = 150.0
        sigma = 25.0
        self._gaussianWorkspace(peak_center, 1000, sigma)
        peak, low_res, primary_range = LRPeakSelection(self.ws, ComputePrimaryRange=True)

        # Require a close relative match between given and fitted values.
        # The width of the fitted peak should be about twice the sigma.
        position = (peak[0] + peak[1]) / 2.0
        width = abs(peak[1] - peak[0]) / 4.0
        diff_peak_centre = abs((position - peak_center) / peak_center)
        diff_sigma = abs((width - sigma) / sigma)

        self.assertLess(diff_peak_centre, 0.03)
        self.assertLess(diff_sigma, 0.05)

        # The low resolution range should fit within the primary range
        self.assertLess(primary_range[0], low_res[0])
        self.assertGreater(primary_range[1], low_res[1])


if __name__ == "__main__":
    unittest.main()
