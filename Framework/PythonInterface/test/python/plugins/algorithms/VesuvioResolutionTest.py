# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import VesuvioResolution
import vesuvio.testing as testing


class VesuvioResolutionTest(unittest.TestCase):
    def setUp(self):
        """
        Create a sample workspace in time of flight.
        """
        self._sample_ws = testing.create_test_ws()

    def test_basic_resolution(self):
        """
        Tests a baisc run of the algorithm.
        """

        tof, ysp = VesuvioResolution(Workspace=self._sample_ws, Mass=1.0079)

        self.assertEqual(tof.getAxis(0).getUnit().unitID(), "TOF")
        self.assertEqual(ysp.getAxis(0).getUnit().unitID(), "Label")

    def test_ws_validation(self):
        """
        Tests that validation fails if no output workspace is given.
        """

        self.assertRaisesRegex(
            RuntimeError, "Must output in either time of flight or ySpace", VesuvioResolution, Workspace=self._sample_ws, Mass=1.0079
        )

    def test_ws_index_validation(self):
        """
        Tests that validation fails if ws index is out of range.
        """

        self.assertRaisesRegex(
            RuntimeError, "Workspace index is out of range", VesuvioResolution, Workspace=self._sample_ws, Mass=1.0079, WorkspaceIndex=50
        )


if __name__ == "__main__":
    unittest.main()
