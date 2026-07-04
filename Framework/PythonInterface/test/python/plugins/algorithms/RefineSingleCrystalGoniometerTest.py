# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.simpleapi import (
    RefineSingleCrystalGoniometer,
    CloneWorkspace,
    LoadIsawPeaks,
    FindUBUsingIndexedPeaks,
    FindUBUsingFFT,
    IndexPeaks,
)


class RefineSingleCrystalGoniometerTest(unittest.TestCase):
    def setUp(self):
        return

    def tearDown(self):
        return

    def testExample(self):
        filename = "TOPAZ_2479.peaks"

        LoadIsawPeaks(Filename=filename, OutputWorkspace="peaks")

        FindUBUsingIndexedPeaks(PeaksWorkspace="peaks", Tolerance=0.12)
        CloneWorkspace(InputWorkspace="peaks", OutputWorkspace="refined")

        index_null = IndexPeaks(PeaksWorkspace="peaks", Tolerance=0.12)

        initial = index_null.NumIndexed

        RefineSingleCrystalGoniometer(Peaks="refined", Tolerance=0.12, Cell="Triclinic", NumIterations=1)

        index_refine = IndexPeaks(PeaksWorkspace="refined", Tolerance=0.12)

        final = index_refine.NumIndexed

        assert final > initial


class RefineSingleCrystalGoniometerTestWithLargeOffset(unittest.TestCase):
    def setUp(self):
        return

    def tearDown(self):
        return

    def testExample(self):
        filename = "TOPAZ_293K_Triclinic_P_unreliable_motors.nxs"

        LoadIsawPeaks(Filename=filename, OutputWorkspace="peaks")

        # IndexPeaks requires an OrientedLattice to already be set, so seed
        # one with a single FindUBUsingFFT across all runs. With unreliable
        # per-run goniometer offsets, this single shared UB indexes poorly.
        FindUBUsingFFT(PeaksWorkspace="peaks", MinD=5, MaxD=15)

        index_null = IndexPeaks(PeaksWorkspace="peaks", Tolerance=0.12)

        initial = index_null.NumIndexed

        # LargeOffset=True indexes each run independently via FindUBUsingFFT
        # (robust to the large per-run misorientation) before refining the
        # UB and goniometer offsets jointly.
        RefineSingleCrystalGoniometer(
            Peaks="peaks",
            Tolerance=0.12,
            Cell="Triclinic",
            NumIterations=8,
            LargeOffset=True,
            MinD=5,
            MaxD=15,
        )

        index_refine = IndexPeaks(PeaksWorkspace="peaks", Tolerance=0.12)

        final = index_refine.NumIndexed

        assert final > initial


if __name__ == "__main__":
    unittest.main()
