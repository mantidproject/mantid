import unittest
from mantid.simpleapi import *
from mantid.api import *
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

        self.assertEqual(tof.getAxis(0).getUnit().unitID(), 'TOF')
        self.assertEqual(ysp.getAxis(0).getUnit().unitID(), 'Label')


    def test_ws_validation(self):
        """
        Tests that validation fails if no output workspace is given.
        """

        self.assertRaises(RuntimeError, VesuvioResolution,
                          Workspace=self._sample_ws, Mass=1.0079)


    def test_ws_index_validation(self):
        """
        Tests that validation fails if ws index is out of range.
        """

        self.assertRaises(RuntimeError, VesuvioResolution,
                          Workspace=self._sample_ws, Mass=1.0079, WorkspaceIndex=50)


if __name__ == '__main__':
    unittest.main()
