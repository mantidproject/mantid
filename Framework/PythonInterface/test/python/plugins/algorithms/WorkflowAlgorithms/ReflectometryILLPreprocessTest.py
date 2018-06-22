# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
import numpy.testing
from testhelpers import (assertRaisesNothing, create_algorithm)
import unittest


class ReflectometryILLPreprocessTest(unittest.TestCase):
    def tearDown(self):
        mtd.clear()

    def testDefaultRunExecutesSuccessfully(self):
        outWSName = 'outWS'
        args = {
            'Run': 'ILL/D17/317370.nxs',
            'OutputWorkspace': outWSName,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getAxis(0).getUnit().caption(), 'Wavelength')

    def testFlatBackgroundSubtraction(self):
        inWSName = 'ReflectometryILLPreprocess_test_ws'
        self.create_sample_workspace(inWSName)
        # Add a peak to the sample workspace.
        ws = mtd[inWSName]
        ys = ws.dataY(49)
        ys += 10.0
        args = {
            'InputWorkspace': inWSName,
            'OutputWorkspace': 'unused_for_child',
            'BeamCentre': 49,
            'FluxNormalisation': 'Normalisation OFF',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getNumberHistograms(), 100)
        for i in range(outWS.getNumberHistograms()):
            ys = outWS.readY(i)
            if i != 49:
                numpy.testing.assert_equal(ys, 0)
            else:
                numpy.testing.assert_almost_equal(ys, 10.)

    def _backgroundSubtraction(self, subtractionType):
        inWSName = 'ReflectometryILLPreprocess_test_ws'
        self.create_sample_workspace(inWSName)
        # Add a peak to the sample workspace.
        ws = mtd[inWSName]
        ys = ws.dataY(49)
        ys += 10.0
        args = {
            'InputWorkspace': inWSName,
            'OutputWorkspace': 'unused_for_child',
            'BeamCentre': 49,
            'FluxNormalisation': 'Normalisation OFF',
            'FlatBackground': subtractionType,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getNumberHistograms(), 100)
        for i in range(outWS.getNumberHistograms()):
            ys = outWS.readY(i)
            if i != 49:
                numpy.testing.assert_almost_equal(ys, 0)
            else:
                numpy.testing.assert_almost_equal(ys, 10.)

    def testLinearFlatBackgroundSubtraction(self):
        self._backgroundSubtraction('Background Linear Fit')

    def testConstantFlatBackgroundSubtraction(self):
        self._backgroundSubtraction('Background Constant Fit')

    def testDisableFlatBackgroundSubtraction(self):
        inWSName = 'ReflectometryILLPreprocess_test_ws'
        self.create_sample_workspace(inWSName)
        # Add a peak to the sample workspace.
        ws = mtd[inWSName]
        bkgLevel = ws.readY(0)[0]
        self.assertGreater(bkgLevel, 0.1)
        args = {
            'InputWorkspace': inWSName,
            'OutputWorkspace': 'unused_for_child',
            'BeamCentre': 49,
            'FluxNormalisation': 'Normalisation OFF',
            'FlatBackground': 'Background OFF',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getNumberHistograms(), 100)
        for i in range(outWS.getNumberHistograms()):
            ys = outWS.readY(i)
            numpy.testing.assert_equal(ys, bkgLevel)

    def testForegroundBackgroundRanges(self):
        inWSName = 'ReflectometryILLPreprocess_test_ws'
        self.create_sample_workspace(inWSName)
        ws = mtd[inWSName]
        # Add special background fitting zones around the exclude zones.
        lowerBkgIndices = [26]
        for i in lowerBkgIndices:
            ys = ws.dataY(i)
            ys += 5.0
        # Add negative 'exclude zone' around the peak.
        lowerExclusionIndices = [27, 28]
        for i in lowerExclusionIndices:
            ys = ws.dataY(i)
            ys -= 1000.0
        # Add a peak to the sample workspace.
        foregroundIndices = [29, 30, 31]
        for i in foregroundIndices:
            ys = ws.dataY(i)
            ys += 1000.0
        # The second exclusion zone is wider.
        upperExclusionIndices = [32, 33, 34]
        for i in upperExclusionIndices:
            ys = ws.dataY(i)
            ys -= 1000.0
        # The second fitting zone is wider.
        upperBkgIndices = [35, 36]
        for i in upperBkgIndices:
            ys = ws.dataY(i)
            ys += 5.0
        args = {
            'InputWorkspace': inWSName,
            'OutputWorkspace': 'unused_for_child',
            'BeamCentre': 30,
            'ForegroundHalfWidth': [1],
            'LowAngleBkgOffset': len(lowerExclusionIndices),
            'LowAngleBkgWidth': len(lowerBkgIndices),
            'HighAngleBkgOffset': len(upperExclusionIndices),
            'HighAngleBkgWidth': len(upperBkgIndices),
            'FluxNormalisation': 'Normalisation OFF',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getNumberHistograms(), 100)
        for i in range(outWS.getNumberHistograms()):
            ys = outWS.readY(i)
            if i in lowerBkgIndices:
                numpy.testing.assert_equal(ys, 0)
            elif i in lowerExclusionIndices:
                numpy.testing.assert_equal(ys, -1005)
            elif i in foregroundIndices:
                numpy.testing.assert_equal(ys, 995)
            elif i in upperExclusionIndices:
                numpy.testing.assert_equal(ys, -1005)
            elif i in upperBkgIndices:
                numpy.testing.assert_equal(ys, 0)
            else:
                numpy.testing.assert_equal(ys, -5)

    def testAsymmetricForegroundRanges(self):
        inWSName = 'ReflectometryILLPreprocess_test_ws'
        self.create_sample_workspace(inWSName)
        ws = mtd[inWSName]
        # Add special background fitting zones around the exclude zones.
        foregroundIndices = [21, 22, 23, 24]
        for i in range(ws.getNumberHistograms()):
            ys = ws.dataY(i)
            es = ws.dataE(i)
            if i in foregroundIndices:
                ys.fill(1000.0)
                es.fill(numpy.sqrt(1000.0))
            else:
                ys.fill(-100)
                es.fill(numpy.sqrt(100))
        args = {
            'InputWorkspace': inWSName,
            'OutputWorkspace': 'unused_for_child',
            'BeamCentre': 23,
            'ForegroundHalfWidth': [2, 1],
            'FlatBackground': 'Background OFF',
            'FluxNormalisation': 'Normalisation OFF',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getNumberHistograms(), 100)
        logs = outWS.run()
        properties = ['foreground.first_workspace_index', 'foreground.centre_workspace_index', 'foreground.last_workspace_index']
        values = [21, 23, 24]
        for p, val in zip(properties, values):
            self.assertTrue(logs.hasProperty(p))
            self.assertEqual(logs.getProperty(p).value, val)

    def testWaterWorkspace(self):
        inWSName = 'ReflectometryILLPreprocess_test_ws'
        self.create_sample_workspace(inWSName)
        # Add a peak to the sample workspace.
        ws = mtd[inWSName]
        for i in range(ws.getNumberHistograms()):
            ys = ws.dataY(i)
            ys.fill(10.27)
        args = {
            'InputWorkspace': inWSName,
            'OutputWorkspace': 'unused_for_child',
            'WaterWorkspace': inWSName,
            'FluxNormalisation': 'Normalisation OFF',
            'FlatBackground': 'Background OFF',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getNumberHistograms(), 100)
        for i in range(outWS.getNumberHistograms()):
            ys = outWS.readY(i)
            numpy.testing.assert_equal(ys, 1.0)

    def testCleanupOFF(self):
        # test if intermediate workspaces exist:
        # not tested: position tables
        # normalise_to_slits, normalise_to_monitor, '_normalised_to_time_','transposed_flat_background'
        workspace_name_suffix = ['_cloned_for_flat_bkg_', '_detectors_', '_flat_background_',
                                 '_flat_background_subtracted_', '_in_wavelength_',
                                 '_monitors_', '_transposed_clone_', '_water_calibrated_']
        outWSName = 'outWS'
        inWSName = 'inWS'
        self.create_sample_workspace(inWSName, NumMonitors=3)
        waterName = 'water'
        self.create_sample_workspace(waterName)

        arg = {'OutputWorkspace': 'peakTable'}
        alg = create_algorithm('CreateEmptyTableWorkspace', **arg)
        alg.execute()
        table = mtd['peakTable']
        table.addColumn('double', 'PeakCentre')
        table.addRow((3.0,))
        args = {
            'InputWorkspace': inWSName,
            'BeamPositionWorkspace': 'peakTable',
            'OutputWorkspace': outWSName,
            'Cleanup': 'Cleanup OFF',
            'WaterWorkspace': waterName,
            'ForegroundHalfWidth': [1],
            'LowAngleBkgOffset': 5,
            'LowAngleBkgWidth': 10,
            'HighAngleBkgOffset': 5,
            'HighAngleBkgWidth': 10,
            'FluxNormalisation': 'Normalisation OFF',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        for i in range(len(workspace_name_suffix)):
            wsName = outWSName + workspace_name_suffix[i]
            self.assertTrue(mtd.doesExist(wsName), wsName)

    def testTwoInputFiles(self):
        outWSName = 'outWS'
        args = {
            'Run': 'ILL/D17/317369, ILL/D17/317370.nxs',
            'OutputWorkspace': outWSName,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getAxis(0).getUnit().caption(), 'Wavelength')

    def create_sample_workspace(self, name, NumMonitors=0):
        args = {
            'OutputWorkspace': name,
            'Function': 'Flat background',
            'NumMonitors': NumMonitors,
            'NumBanks': 1,
        }
        alg = create_algorithm('CreateSampleWorkspace', **args)
        alg.setLogging(False)
        alg.execute()


if __name__ == "__main__":
    unittest.main()
