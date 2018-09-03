#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import CreateFloodWorkspace, CreateWorkspace, SaveNexus, mtd
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


class ReflectometryCreateFloodWorkspaceCentralPixel(stresstesting.MantidStressTest):

    flood_ws_name = 'flood'

    def runTest(self):
        try:
            input_file = tempfile.gettempdir() + '/__refl_flood_cor_temp.nxs'
            x = [0, 100000, 0, 100000, 0, 100000, 0, 100000, 0, 100000, 0, 100000]
            y = [1, 9, 8, 3, 14, 15]
            ws = CreateWorkspace(x, y, NSpec=6)
            SaveNexus(ws, input_file)
            CreateFloodWorkspace(input_file, CentralPixelSpectrum=2, OutputWorkspace=self.flood_ws_name)
            out = mtd[self.flood_ws_name]
            self.assertAlmostEqual(out.readY(0)[0], 1.0/8)
            self.assertAlmostEqual(out.readY(1)[0], 9.0/8)
            self.assertAlmostEqual(out.readY(2)[0], 1.0)
            self.assertAlmostEqual(out.readY(3)[0], 3.0/8)
            self.assertAlmostEqual(out.readY(4)[0], 14.0/8)
            self.assertAlmostEqual(out.readY(5)[0], 15.0/8)
        finally:
            os.unlink(input_file)


class ReflectometryCreateFloodWorkspaceIntegrationRange(stresstesting.MantidStressTest):

    flood_ws_name = 'flood'

    def runTest(self):
        try:
            input_file = tempfile.gettempdir() + '/__refl_flood_cor_temp.nxs'
            x = [0, 5, 6, 10] * 6
            y = [1, 2, 1] + [9, 3, 9] + [8, 4, 8] + [3, 5, 3] + [14, 6, 14] + [15, 7, 15]
            ws = CreateWorkspace(x, y, NSpec=6)
            SaveNexus(ws, input_file)
            CreateFloodWorkspace(input_file, CentralPixelSpectrum=2, RangeLower=3, RangeUpper=7, OutputWorkspace=self.flood_ws_name)
            out = mtd[self.flood_ws_name]
            self.assertAlmostEqual(out.readY(0)[0], 2.0/4)
            self.assertAlmostEqual(out.readY(1)[0], 3.0/4)
            self.assertAlmostEqual(out.readY(2)[0], 1.0)
            self.assertAlmostEqual(out.readY(3)[0], 5.0/4)
            self.assertAlmostEqual(out.readY(4)[0], 6.0/4)
            self.assertAlmostEqual(out.readY(5)[0], 7.0/4)
        finally:
            os.unlink(input_file)
