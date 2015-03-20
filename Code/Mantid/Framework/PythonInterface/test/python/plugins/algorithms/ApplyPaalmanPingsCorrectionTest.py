import unittest
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import Load, ConvertUnits, CreateSimulationWorkspace, RebinToWorkspace, Scale, DeleteWorkspace


class ApplyPaalmanPingsCorrectionTest(unittest.TestCase):

    def setUp(self):
        """
        Create sample workspaces.
        """

        # Load the sample and can
        sample_ws = Load('irs26176_graphite002_red.nxs')
        can_ws = Load('irs26173_graphite002_red.nxs')

        # Convert sample and can to wavelength
        sample_ws = ConvertUnits(InputWorkspace=sample_ws,
                                 Target='Wavelength',
                                 EMode='Indirect',
                                 EFixed=1.845)
        can_ws = ConvertUnits(InputWorkspace=can_ws,
                              Target='Wavelength',
                              EMode='Indirect',
                              EFixed=1.845)

        self._sample_ws = sample_ws
        self._can_ws = can_ws

        # Create a dummy correction workspace roughtly similar to the sample
        ws = CreateSimulationWorkspace(Instrument='IRIS',
                                       BinParams='6,0.1,8',
                                       UnitX='Wavelength')

        # Rebin the dummy workspace to match the smaple
        ws = RebinToWorkspace(WorkspaceToRebin=ws,
                              WorkspaceToMatch=sample_ws)

        # Test correction workspace names
        self._ass_ws_name = '__ApplyPaalmanPingsCorrection_ass'
        self._acc_ws_name = '__ApplyPaalmanPingsCorrection_acc'
        self._acsc_ws_name = '__ApplyPaalmanPingsCorrection_acsc'
        self._assc_ws_name = '__ApplyPaalmanPingsCorrection_assc'

        # Scale them to make them look like correction factors
        Scale(InputWorkspace=ws,
              Factor=0.54,
              Operation='Multiply',
              OutputWorkspace=self._ass_ws_name)

        Scale(InputWorkspace=ws,
              Factor=0.9,
              Operation='Multiply',
              OutputWorkspace=self._acc_ws_name)

        Scale(InputWorkspace=ws,
              Factor=0.54,
              Operation='Multiply',
              OutputWorkspace=self._acsc_ws_name)

        Scale(InputWorkspace=ws,
              Factor=0.56,
              Operation='Multiply',
              OutputWorkspace=self._assc_ws_name)

        # Delete the dummy workspace
        DeleteWorkspace(ws)


    def tearDown(self):
        """
        Remove workspaces from ADS.
        """

        # Sample and can
        DeleteWorkspace(self._sample_ws)
        DeleteWorkspace(self._can_ws)

        # Simulated corrections
        DeleteWorkspace(self._ass_ws_name)
        DeleteWorkspace(self._acc_ws_name)
        DeleteWorkspace(self._acsc_ws_name)
        DeleteWorkspace(self._assc_ws_name)


    def _verify_workspace(self, ws_name):
        """
        Do validation on a correction workspace.

        @param ws_name Name of workspace to validate
        """

        pass  # TODO


    def test(self):
        """
        """

        pass  # TODO


if __name__=="__main__":
    unittest.main()
