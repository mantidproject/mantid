# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import Load, FindUBUsingIndexedPeaks, IndexPeaks
import unittest
import numpy as np
import numpy.testing as npt


class IndexPeaksTest(unittest.TestCase):
    """
    The purpose of the testing is to ensure the Python bindning works
    with various different input arguments.
    """
    def test_exec_with_different_args(self):
        # load data
        Load(
            Filename="test_index_satellite_peaks.integrate",
            OutputWorkspace="test",
        )
        FindUBUsingIndexedPeaks(PeaksWorkspace="test")

        # defualt args
        IndexPeaks(PeaksWorkspace="test", Tolerance=0.12)

        # testing zero (default) modVec with maxOrder=1
        IndexPeaks(
            PeaksWorkspace="test",
            Tolerance=0.12,
            RoundHKLs=False,
            SaveModulationInfo=True,
            MaxOrder=1,
        )

        # testing one non-zero modVec with maxOrder=1
        IndexPeaks(
            PeaksWorkspace="test",
            Tolerance=0.12,
            RoundHKLs=False,
            SaveModulationInfo=True,
            MaxOrder=1,
            ModVector1="0,0,0.33333",
        )

        # testing one non-zero modVec with maxOrder=0
        IndexPeaks(
            PeaksWorkspace="test",
            Tolerance=0.12,
            RoundHKLs=False,
            SaveModulationInfo=True,
            ModVector1="0,0,0.33333",
        )


if __name__ == "__main__":
    unittest.main()
