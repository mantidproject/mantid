# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import (ReflectometryILLPreprocess)
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
            'rethrow': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)

    def testFlatBackgroundSubtraction(self):
        inWSName = 'ReflectometryILLPreprocess_test_ws'
        args = {
            'OutputWorkspace': inWSName,
            'Function': 'Flat background',
            'NumBanks': 1,
        }
        alg = create_algorithm('CreateSampleWorkspace', **args)
        alg.execute()
        # Add a peak to the sample workspace.
        ws = mtd[inWSName]
        ys = ws.dataY(49)
        ys += 10.0
        args = {
            'InputWorkspace': inWSName,
            'OutputWorkspace': 'unused_for_child',
            'ForegroundCentre': 50,
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
                xs = outWS.readX(i)
                dx = xs[1] - xs[0]
                numpy.testing.assert_almost_equal(ys, 10 / dx)

    def testForegroundBackgroundRanges(self):
        inWSName = 'ReflectometryILLPreprocess_test_ws'
        args = {
            'OutputWorkspace': inWSName,
            'Function': 'Flat background',
            'NumBanks': 1,
        }
        alg = create_algorithm('CreateSampleWorkspace', **args)
        alg.execute()
        ws = mtd[inWSName]
        # Add special background fitting zones around the exclude zones.
        upperBkgIndices = [26]
        for i in upperBkgIndices:
            ys = ws.dataY(i)
            ys += 5.0
        # Add negative 'exclude zone' around the peak.
        upperExclusionIndices = [27, 28]
        for i in upperExclusionIndices:
            ys = ws.dataY(i)
            ys -= 1000.0
        # Add a peak to the sample workspace.
        foregroundIndices = [29, 30, 31]
        for i in foregroundIndices:
            ys = ws.dataY(i)
            ys += 1000.0
        # The second exclusion zone is wider.
        lowerExclusionIndices = [32, 33, 34]
        for i in lowerExclusionIndices:
            ys = ws.dataY(i)
            ys -= 1000.0
        # The second fittin zone is wider.
        lowerBkgIndices = [35, 36]
        for i in lowerBkgIndices:
            ys = ws.dataY(i)
            ys += 5.0
        args = {
            'InputWorkspace': inWSName,
            'OutputWorkspace': 'unused_for_child',
            'SumOutput': 'Summation OFF',
            'ForegroundCentre': 31,
            'ForegroundHalfWidth': 1,
            'LowerBackgroundOffset': len(lowerExclusionIndices),
            'LowerBackgroundWidth': len(lowerBkgIndices),
            'UpperBackgroundOffset': len(upperExclusionIndices),
            'UpperBackgroundWidth': len(upperBkgIndices),
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
            if i in upperBkgIndices:
                numpy.testing.assert_equal(ys, 0)
            elif i in upperExclusionIndices:
                numpy.testing.assert_equal(ys, -1005)
            elif i in foregroundIndices:
                numpy.testing.assert_equal(ys, 995)
            elif i in lowerExclusionIndices:
                numpy.testing.assert_equal(ys, -1005)
            elif i in lowerBkgIndices:
                numpy.testing.assert_equal(ys, 0)
            else:
                numpy.testing.assert_equal(ys, -5)

    def testWaterReference(self):
        inWSName = 'ReflectometryILLPreprocess_test_ws'
        args = {
            'OutputWorkspace': inWSName,
            'Function': 'Flat background',
            'NumBanks': 1,
        }
        alg = create_algorithm('CreateSampleWorkspace', **args)
        alg.execute()
        # Add a peak to the sample workspace.
        ws = mtd[inWSName]
        for i in range(ws.getNumberHistograms()):
            ys = ws.dataY(i)
            ys.fill(10.27)
        args = {
            'InputWorkspace': inWSName,
            'OutputWorkspace': 'unused_for_child',
            'WaterReference': inWSName,
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


    def testWavelengthRange(self):
        inWSName = 'ReflectometryILLPreprocess_test_ws'
        args = {
            'OutputWorkspace': inWSName,
            'Function': 'Flat background',
            'NumBanks': 1,
        }
        alg = create_algorithm('CreateSampleWorkspace', **args)
        alg.execute()
        # Check range begin only.
        args = {
            'InputWorkspace': inWSName,
            'OutputWorkspace': 'unused_for_child',
            'WavelengthRange': [2.0],
            'ForegroundCentre': 50,
            'ForegroundHalfWidth': 1,
            'FluxNormalisation': 'Normalisation OFF',
            'FlatBackground': 'Background OFF',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getNumberHistograms(), 1)
        self.assertAlmostEquals(outWS.readX(0)[0], 2.0, places=2)
        # Check both range begin and end.
        args = {
            'InputWorkspace': inWSName,
            'OutputWorkspace': 'unused_for_child',
            'WavelengthRange': [2.0, 4.0],
            'ForegroundCentre': 50,
            'ForegroundHalfWidth':1,
            'FluxNormalisation': 'Normalisation OFF',
            'FlatBackground': 'Background OFF',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEquals(outWS.getNumberHistograms(), 1)
        xs = outWS.readX(0)
        self.assertAlmostEquals(xs[0], 2.0, places=2)
        self.assertAlmostEquals(xs[-1], 4.0, places=1)


if __name__ == "__main__":
    unittest.main()
