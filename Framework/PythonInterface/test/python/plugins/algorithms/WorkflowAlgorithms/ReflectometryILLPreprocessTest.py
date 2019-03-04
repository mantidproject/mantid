# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import ReflectometryILLPreprocess
import numpy.testing
from testhelpers import (assertRaisesNothing, create_algorithm, illhelpers)
import unittest
import ReflectometryILL_common as common


class ReflectometryILLPreprocessTest(unittest.TestCase):
    def tearDown(self):
        mtd.clear()

    def testDefaultRunD17(self):
        args = {
            'Run': 'ILL/D17/317370.nxs',
            'OutputWorkspace': 'outWS',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getAxis(0).getUnit().caption(), 'Wavelength')

        det122 = outWS.getDetector(122)
        twoTheta122 = outWS.detectorTwoTheta(det122)
        self.assertAlmostEquals(twoTheta122, 0.057659886309975004, delta=1.e-13)

        twoTheta = outWS.run().getProperty(common.SampleLogs.TWO_THETA).value
        self.assertAlmostEquals(twoTheta, 3.182191848754883, delta=1.e-13)

        peakPosition = outWS.run().getProperty(common.SampleLogs.FOREGROUND_CENTRE).value
        twoTheta2 = numpy.rad2deg(outWS.spectrumInfo().twoTheta(peakPosition))
        self.assertAlmostEquals(twoTheta2, 1.5371900428796088, delta=1.e-13)
        self.assertEquals(mtd.getObjectNames(), [])

    def testDefaultRunFIGARO(self):
        args = {
            'Run': 'ILL/Figaro/000002.nxs',
            'OutputWorkspace': 'outWS',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getAxis(0).getUnit().caption(), 'Wavelength')

        det122 = outWS.getDetector(122)
        twoTheta122 = outWS.detectorTwoTheta(det122)
        self.assertAlmostEquals(twoTheta122, 0.03060663990053301, delta=1.e-13)
        self.assertEquals(mtd.getObjectNames(), [])

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
            'LinePosition': 49,
            'FluxNormalisation': 'Normalisation OFF',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getNumberHistograms(), 100)
        ysSize = outWS.blocksize()
        for i in range(outWS.getNumberHistograms()):
            ys = outWS.readY(i)
            if i != 49:
                numpy.testing.assert_equal(ys, [0.0] * ysSize)
            else:
                numpy.testing.assert_almost_equal(ys, [10.0] * ysSize)
        self.assertEquals(mtd.getObjectNames(), ['ReflectometryILLPreprocess_test_ws'])

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
            'LinePosition': 49,
            'FluxNormalisation': 'Normalisation OFF',
            'FlatBackground': subtractionType,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getNumberHistograms(), 100)
        ysSize = outWS.blocksize()
        for i in range(outWS.getNumberHistograms()):
            ys = outWS.readY(i)
            if i != 49:
                numpy.testing.assert_almost_equal(ys, [0.0] * ysSize)
            else:
                numpy.testing.assert_almost_equal(ys, [10.0] * ysSize)
        self.assertEquals(mtd.getObjectNames(), ['ReflectometryILLPreprocess_test_ws'])

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
            'LinePosition': 49,
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
        self.assertEquals(mtd.getObjectNames(), ['ReflectometryILLPreprocess_test_ws'])

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
            'LinePosition': 30,
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
        ysSize = outWS.blocksize()
        for i in range(outWS.getNumberHistograms()):
            ys = outWS.readY(i)
            if i in lowerBkgIndices:
                numpy.testing.assert_equal(ys, [0.0] * ysSize)
            elif i in lowerExclusionIndices:
                numpy.testing.assert_equal(ys, [-1005.0] * ysSize)
            elif i in foregroundIndices:
                numpy.testing.assert_equal(ys, [995.0] * ysSize)
            elif i in upperExclusionIndices:
                numpy.testing.assert_equal(ys, [-1005.0] * ysSize)
            elif i in upperBkgIndices:
                numpy.testing.assert_equal(ys, [0.0] * ysSize)
            else:
                numpy.testing.assert_equal(ys, [-5.0] * ysSize)
        self.assertEquals(mtd.getObjectNames(), ['ReflectometryILLPreprocess_test_ws'])

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
            'LinePosition': 23,
            'TwoTheta': 0.6,
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
        properties = [common.SampleLogs.FOREGROUND_START,
                      common.SampleLogs.FOREGROUND_CENTRE,
                      common.SampleLogs.FOREGROUND_END]
        values = [21, 23, 24]
        for p, val in zip(properties, values):
            self.assertTrue(logs.hasProperty(p))
            self.assertEqual(logs.getProperty(p).value, val)
        self.assertEquals(mtd.getObjectNames(), ['ReflectometryILLPreprocess_test_ws'])

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
        ysSize = outWS.blocksize()
        for i in range(outWS.getNumberHistograms()):
            ys = outWS.readY(i)
            numpy.testing.assert_equal(ys, [1.0] * ysSize)
        self.assertEquals(mtd.getObjectNames(), ['ReflectometryILLPreprocess_test_ws'])

    def testCleanupOFF(self):
        # test if intermediate workspaces exist:
        # normalise_to_slits, normalise_to_monitor, '_normalised_to_time_','transposed_flat_background'
        outWSName = 'outWS'
        ws = illhelpers.create_poor_mans_d17_workspace()
        ws = illhelpers.refl_add_line_position(ws, 3.0)
        self.assertEquals(ws.run().getProperty(common.SampleLogs.LINE_POSITION).value, 3.0)
        # Add a peak to the workspace.
        for i in range(33, 100):
            ys = ws.dataY(i)
            ys += 10.0
        args = {
            'InputWorkspace': ws,
            'OutputWorkspace': outWSName,
            'TwoTheta': 0.6,
            'Cleanup': 'Cleanup OFF',
            'WaterWorkspace': ws,
            'ForegroundHalfWidth': [1, 2],
            'FluxNormalisation': 'Normalisation OFF',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        wsInADS = mtd.getObjectNames()
        self.assertEquals(len(wsInADS), 13)
        self.assertEquals(wsInADS, [
            'outWS_cloned_for_flat_bkg_',
            'outWS_detectors_',
            'outWS_detectors_moved_',
            'outWS_flat_background_',
            'outWS_flat_background_subtracted_',
            'outWS_in_wavelength_',
            'outWS_monitors_',
            'outWS_peak_',
            'outWS_transposed_clone_',
            'outWS_transposed_flat_background_',
            'outWS_water_calibrated_',
            'outWS_water_detectors_',
            'outWS_water_rebinned_']
                          )
        mtd.clear()

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
        self.assertEquals(mtd.getObjectNames(), [])

    def create_sample_workspace(self, name, numMonitors=0):
        args = {
            'OutputWorkspace': name,
            'Function': 'Flat background',
            'NumMonitors': numMonitors,
            'NumBanks': 1,
        }
        alg = create_algorithm('CreateSampleWorkspace', **args)
        alg.setLogging(False)
        alg.execute()
        loadInstrArgs = {
            'Workspace': name,
            'InstrumentName': 'FIGARO',
            'RewriteSpectraMap': False
        }
        loadInstrument = create_algorithm('LoadInstrument', **loadInstrArgs)
        loadInstrument.setLogging(False)
        loadInstrument.execute()
        addSampleLogArgs = {
            'Workspace': name,
            'LogType': 'Number',
            'LogName': common.SampleLogs.TWO_THETA,
            'NumberType': 'Double',
            'LogText': '5.6',
            'LogUnit': 'degree'
        }
        addSampleLog = create_algorithm('AddSampleLog', **addSampleLogArgs)
        addSampleLog.setLogging(False)
        addSampleLog.execute()
        self.assertEquals(mtd.getObjectNames(), [name])

    def testLinePositionTwoThetaInput(self):
        args = {
            'Run': 'ILL/D17/317369',
            'LinePosition': 10.23,
            'TwoTheta': 40.66,
            'OutputWorkspace': 'outWS',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getRun().getProperty(common.SampleLogs.LINE_POSITION).value, 10.23)
        self.assertEquals(outWS.getRun().getProperty(common.SampleLogs.FOREGROUND_CENTRE).value, 10)
        self.assertEquals(outWS.getRun().getProperty(common.SampleLogs.TWO_THETA).value, 1.5885926485061646)
        self.assertEquals(outWS.getAxis(0).getUnit().caption(), 'Wavelength')
        self.assertEquals(mtd.getObjectNames(), [])

    def testTwoThetaFit(self):
        args = {
            'Run': 'ILL/D17/317369',
            'OutputWorkspace': 'outWS',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertAlmostEquals(outWS.getRun().getProperty(common.SampleLogs.LINE_POSITION).value,
                                202.1773407538167,
                                delta=1.e-13)
        self.assertEquals(outWS.getRun().getProperty(common.SampleLogs.FOREGROUND_CENTRE).value, 202)
        self.assertEquals(outWS.getAxis(0).getUnit().caption(), 'Wavelength')
        print(mtd.getObjectNames())

    def testUnitConversion(self):
        args = {
            'Run': 'ILL/D17/317369',
            'OutputWorkspace': 'outWS',
            'FitRangeLower': 2.,
            'FitRangeUpper': 20.,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertAlmostEquals(outWS.getRun().getProperty(common.SampleLogs.LINE_POSITION).value,
                                202.17752545515665,
                                delta=1.e-13)
        self.assertEquals(outWS.getRun().getProperty(common.SampleLogs.FOREGROUND_CENTRE).value, 202)
        self.assertEquals(outWS.getAxis(0).getUnit().caption(), 'Wavelength')

    def testDirectBeamInputForDetectorRotation(self):
        direct = ReflectometryILLPreprocess(Run='ILL/D17/317369',
                                            TwoTheta=60.0,
                                            LinePosition=101.2)
        self.assertEquals(direct.run().getProperty(common.SampleLogs.LINE_POSITION).value, 101.2)
        # We expect here a default detector rotation angle
        self.assertEquals(direct.run().getProperty(common.SampleLogs.TWO_THETA).value, 1.5885926485061646)
        self.assertEquals(direct.getAxis(0).getUnit().caption(), 'Wavelength')
        reflected = ReflectometryILLPreprocess(Run='ILL/D17/317370',
                                               TwoTheta=80.0,
                                               DirectLineWorkspace=direct)
        self.assertEquals(reflected.run().getProperty(common.SampleLogs.LINE_POSITION).value, 201.6745481268582)
        self.assertEquals(reflected.run().getProperty(common.SampleLogs.TWO_THETA).value, 3.182191848754883)
        self.assertEquals(reflected.run().getProperty(common.SampleLogs.TWO_THETA).units, 'degree')
        self.assertEquals(reflected.getAxis(0).getUnit().caption(), 'Wavelength')


if __name__ == "__main__":
    unittest.main()
