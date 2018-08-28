#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import CreateFloodWorkspace, CreateWorkspace, SaveNexus
import os
import tempfile


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


class ReflectometryCreateFloodWorkspaceNegativeBackground(stresstesting.MantidStressTest):

    flood_ws_name = 'flood'

    def runTest(self):
        try:
            input_file = tempfile.gettempdir() + '/__refl_flood_cor_temp.nxs'
            x = [0, 100000, 0, 100000, 0, 100000, 0, 100000, 0, 100000, 0, 100000]
            y = [1, 9, 8, 3, 1, 1]
            ws = CreateWorkspace(x, y, NSpec=6)
            SaveNexus(ws, input_file)
            self.assertRaises(RuntimeError, CreateFloodWorkspace, input_file, Background='Quadratic', OutputWorkspace=self.flood_ws_name)
        finally:
            os.unlink(input_file)


class ReflectometryApplyFloodWorkspace(stresstesting.MantidStressTest):

    out_ws_name = 'out'

    def runTest(self):
        flood = CreateFloodWorkspace('OFFSPEC00035946.nxs', StartSpectrumIndex=250, EndSpectrumIndex=600,
                                     Exclude=[260, 261, 262, 516, 517, 518], OutputWorkspace='flood')
        data = Load('OFFSPEC00044998.nxs')
        data = Rebin(data, [0,1000,100000], PreserveEvents=False)
        ApplyFloodWorkspace(InputWorkspace=data, FloodWorkspace=flood, OutputWorkspace=self.out_ws_name)

    def validate(self):
        self.disableChecking.append('Instrument')
        return self.out_ws_name,'ReflectometryApplyFloodWorkspace.nxs'
