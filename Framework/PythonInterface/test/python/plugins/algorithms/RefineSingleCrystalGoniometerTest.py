# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import numpy as np

from mantid.simpleapi import mtd, RefineSingleCrystalGoniometer, LoadIsawPeaks, FindUBUsingIndexedPeaks, IndexPeaks


class RefineSingleCrystalGoniometerTest(unittest.TestCase):
    def setUp(self):
        return

    def tearDown(self):
        return

    def testExample(self):
        filename = "/SNS/TOPAZ/IPTS-33878/shared/RFMBA2PbI4/RFMBA2PbI4_mantid_295K_find_peaks/RFMBA2PbI4_Monoclinic_P_5sig.integrate"

        LoadIsawPeaks(Filename=filename, OutputWorkspace="peaks")
        FindUBUsingIndexedPeaks(PeaksWorkspace="peaks")
        IndexPeaks(PeaksWorkspace="peaks", CommonUBForAll=True)

        RefineSingleCrystalGoniometer("peaks", tol=0.25, cell="Monoclinic", n_iter=5)
        IndexPeaks(PeaksWorkspace="peaks", CommonUBForAll=True)

        assert mtd["peaks_#0"]
        assert mtd["peaks_#0"].rowCount() == 20
        assert mtd["peaks_#0"].columnCount() == 6
        assert np.isclose(mtd["peaks_#0"].column(0)[0], -0.12999999523162842)

        assert mtd["peaks_#1"]
        assert mtd["peaks_#1"].rowCount() == 20
        assert mtd["peaks_#1"].columnCount() == 6
        assert np.isclose(mtd["peaks_#1"].column(0)[0], 0.6909528374671936)

        assert mtd["peaks_#2"]
        assert mtd["peaks_#2"].rowCount() == 20
        assert mtd["peaks_#2"].columnCount() == 6
        assert np.isclose(mtd["peaks_#2"].column(0)[0], 1.3717014789581299)

        assert mtd["peaks_#3"]
        assert mtd["peaks_#3"].rowCount() == 20
        assert mtd["peaks_#3"].columnCount() == 6
        assert np.isclose(mtd["peaks_#3"].column(0)[0], 1.9151899814605713)

        assert mtd["peaks_#4"]
        assert mtd["peaks_#4"].rowCount() == 20
        assert mtd["peaks_#4"].columnCount() == 6
        assert np.isclose(mtd["peaks_#4"].column(0)[0], 2.3677234649658203)


if __name__ == "__main__":
    unittest.main()
