# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import mtd
from mantid.simpleapi import CreateWorkspace, DeleteWorkspace, NormaliseToUnity


class NormaliseToUnityTest(unittest.TestCase):
    """
    Simple test to check the numpy integration
    """

    def setUp(self):
        CreateWorkspace(
            [1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6],
            [1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
            [1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
            2,
            OutputWorkspace="normalise_to_unity_test",
        )

    def tearDown(self):
        if mtd.doesExist("normalise_to_unity_test"):
            DeleteWorkspace("normalise_to_unity_test")

    def test_whole_ws(self):
        """
        Check that we can normalize to the sum of all bins
        """
        output_ws = NormaliseToUnity("normalise_to_unity_test")
        self.assertEqual(output_ws.readY(0)[0], 0.1)
        if output_ws:
            DeleteWorkspace(output_ws)

    def test_x_range(self):
        """
        Check that we can specify a range in X and normalize to the sum in that range only
        """
        output_ws = NormaliseToUnity("normalise_to_unity_test", RangeLower=2, RangeUpper=4)
        self.assertEqual(output_ws.readY(0)[0], 0.25)
        if output_ws:
            DeleteWorkspace(output_ws)

    def test_x_range_and_spectra(self):
        """
        Check that we can specify both a range in X and a spectrum range
        """
        output_ws = NormaliseToUnity("normalise_to_unity_test", RangeLower=2, RangeUpper=4, StartWorkspaceIndex=0, EndWorkspaceIndex=0)
        self.assertEqual(output_ws.readY(0)[0], 0.5)
        if output_ws:
            DeleteWorkspace(output_ws)


if __name__ == "__main__":
    unittest.main()
