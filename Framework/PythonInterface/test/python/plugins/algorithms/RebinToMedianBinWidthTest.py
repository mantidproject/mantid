from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import CreateWorkspace, DeleteWorkspace
import numpy
import testhelpers
import unittest

class RebinToMedianBinWidthTest(unittest.TestCase):
    _OUT_WS_NAME = '__RebinToMedianBinWidthTest_outWs'

    def _make_algorithm_params(self, ws, rounding='None'):
        return {
            'InputWorkspace': ws,
            'OutputWorkspace': self._OUT_WS_NAME,
            'rethrow': True, # Let exceptions through for testing.
            'Rounding': rounding
        }

    def _make_boundaries(self, xBegin, binWidths):
        return numpy.cumsum(numpy.append(numpy.array([xBegin]), binWidths))

    def _run_algorithm(self, params):
        algorithm = testhelpers.create_algorithm('RebinToMedianBinWidth', **params)
        testhelpers.assertRaisesNothing(self, algorithm.execute)
        self.assertTrue(algorithm.isExecuted())

    def test_success_single_histogram(self):
        binWidths = numpy.array([0.5, 0.5, 2.3, 2.3, 2.3, 5.9])
        xs = self._make_boundaries(-3.33, binWidths)
        ys = numpy.zeros(len(xs) - 1)
        ws = CreateWorkspace(DataX=xs, DataY=ys)
        params = self._make_algorithm_params(ws)
        self._run_algorithm(params)
        expectedBinWidth = numpy.median(binWidths)
        outWs = mtd[self._OUT_WS_NAME]
        for i in range(outWs.getNumberHistograms()):
            binnedXs = outWs.readX(i)
            newBins = binnedXs[1:] - binnedXs[:-1]
            for binWidth in newBins:
                self.assertAlmostEqual(binWidth, expectedBinWidth)

    def test_average_over_multiple_histograms(self):
        binWidths = numpy.array([0.5, 0.5, 2.3, 2.3, 2.3, 6.5,
                                 0.4, 1.3, 0.4, 1.3, 2.5, 1.3])
        xs1 = self._make_boundaries(-6.6, binWidths[:6])
        xs2 = self._make_boundaries(99.6, binWidths[6:])
        xs = numpy.concatenate((xs1, xs2))
        ys = numpy.zeros(len(xs - 2))
        ws = CreateWorkspace(DataX=xs, DataY=ys, NSpec=2)
        params = self._make_algorithm_params(ws)
        self._run_algorithm(params)
        expectedBinWidth = 0.5 * (2.3 + 1.3)
        outWs = mtd[self._OUT_WS_NAME]
        for i in range(outWs.getNumberHistograms()):
            binnedXs = outWs.readX(i)
            newBins = binnedXs[1:] - binnedXs[:-1]
            for binWidth in newBins:
                self.assertAlmostEqual(binWidth, expectedBinWidth)

    def test_rounding(self):
        binWidths = numpy.array([0.5, 6.1, 2.3, 0.5, 2.3, 2.3])
        xs = self._make_boundaries(-3.33, binWidths)
        ys = numpy.zeros(len(xs) - 1)
        ws = CreateWorkspace(DataX=xs, DataY=ys)
        params = self._make_algorithm_params(ws, rounding='10^n')
        self._run_algorithm(params)
        expectedBinWidth = 1.0
        outWs = mtd[self._OUT_WS_NAME]
        for i in range(outWs.getNumberHistograms()):
            binnedXs = outWs.readX(i)
            newBins = binnedXs[1:] - binnedXs[:-1]
            for binWidth in newBins:
                self.assertAlmostEqual(binWidth, expectedBinWidth)

if __name__ == "__main__":
    unittest.main()
