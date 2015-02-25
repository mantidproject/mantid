import unittest
from mantid.simpleapi import *
from mantid.api import *

class VesuvioResolutionTest(unittest.TestCase):

    def setUp(self):
        """
        Create a sample workspace in time of flight.
        """

        tof_ws = CreateSimulationWorkspace(Instrument='Vesuvio', BinParams=[50,0.5,562], UnitX='TOF')
        tof_ws = CropWorkspace(tof_ws, StartWorkspaceIndex=135, EndWorkspaceIndex=135) # index one less than spectrum number
        tof_ws = ConvertToPointData(tof_ws)
        SetInstrumentParameter(tof_ws, ParameterName='t0', ParameterType='Number', Value='0.5')
        self._sample_ws = tof_ws


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


    def test_spec_index_validation(self):
        """
        Tests that validation fails if spectra index is out of range.
        """

        self.assertRaises(RuntimeError, VesuvioResolution,
                          Workspace=self._sample_ws, Mass=1.0079, SpectrumIndex=50)


if __name__ == '__main__':
    unittest.main()
