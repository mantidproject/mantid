# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.simpleapi import RefineSingleCrystalGoniometer, LoadIsawPeaks, FindUBUsingIndexedPeaks, IndexPeaks


class RefineSingleCrystalGoniometerTest(unittest.TestCase):
    def setUp(self):
        return

    def tearDown(self):
        return

    def testExample(self):
        filename = "TOPAZ_2479.peaks"

        LoadIsawPeaks(Filename=filename, OutputWorkspace="peaks")

        FindUBUsingIndexedPeaks(PeaksWorkspace="peaks", Tolerance=0.12)
        index_null = IndexPeaks(PeaksWorkspace="peaks", Tolerance=0.12)

        initial = index_null.NumIndexed

        RefineSingleCrystalGoniometer("peaks", 0.12, "Triclinic", 1)

        index_refine = IndexPeaks(PeaksWorkspace="peaks", Tolerance=0.12)

        final = index_refine.NumIndexed

        assert final > initial


if __name__ == "__main__":
    unittest.main()
