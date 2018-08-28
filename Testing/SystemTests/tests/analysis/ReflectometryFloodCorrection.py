#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import CreateFloodWorkspace


class ReflectometryCreateFloodWorkspaceNoExclude(stresstesting.MantidStressTest):

    flood_ws_name = 'flood'

    def runTest(self):
        CreateFloodWorkspace('OFFSPEC00035946.nxs', StartSpectrumIndex=265, EndSpectrumIndex=500, OutputWorkspace=self.flood_ws_name)

    def validate(self):
        self.disableChecking.append('Instrument')
        return self.flood_ws_name,'ReflectometryCreateFloodWorkspaceNoExclude.nxs'


class ReflectometryCreateFloodWorkspaceExclude(stresstesting.MantidStressTest):

    flood_ws_name = 'flood'

    def runTest(self):
        CreateFloodWorkspace('OFFSPEC00035946.nxs', StartSpectrumIndex=250, EndSpectrumIndex=600,
                             Exclude=[260, 261, 262, 516, 517, 518], OutputWorkspace=self.flood_ws_name)

    def validate(self):
        self.disableChecking.append('Instrument')
        return self.flood_ws_name,'ReflectometryCreateFloodWorkspaceExclude.nxs'


class ReflectometryCreateFloodWorkspaceQuadratic(stresstesting.MantidStressTest):

    flood_ws_name = 'flood'

    def runTest(self):
        CreateFloodWorkspace('OFFSPEC00035946.nxs', StartSpectrumIndex=10, Background='Quadratic',
                             Exclude=[260, 261, 262, 516, 517, 518], OutputWorkspace=self.flood_ws_name)

    def validate(self):
        self.disableChecking.append('Instrument')
        return self.flood_ws_name,'ReflectometryCreateFloodWorkspaceQuadratic.nxs'
