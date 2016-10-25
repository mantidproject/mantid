from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import CreateWorkspace, DeleteWorkspace
import numpy
import sys
import testhelpers
import unittest

class RebinToBinWidthAtXTest(unittest.TestCase):
    _OUT_WS_NAME = '__rebinToBinWidthAtXTest_outWs'

    def _check_bin_widths(self, expectedBinWidth):
        outWs = mtd[self._OUT_WS_NAME]
        for i in range(outWs.getNumberHistograms()):
            binnedXs = outWs.readX(i)
            newBins = binnedXs[1:] - binnedXs[:-1]
            for binWidth in newBins:
                self.assertAlmostEqual(binWidth, expectedBinWidth)

    def _make_algorithm_params(self, ws, x, rounding='None'):
        return {
            'InputWorkspace': ws,
            'OutputWorkspace': self._OUT_WS_NAME,
            'X': x,
            'rethrow': True, # Let exceptions through for testing.
            'Rounding': rounding
        }

    def _make_boundaries(self, xBegin, binWidths):
        return numpy.cumsum(numpy.append(numpy.array([xBegin]), binWidths))

    def _make_single_histogram_ws(self):
        # X-axis width is a multiple of the final bin width so rebinning
        # creates full bins only.
        binWidths = numpy.array([0.13, 0.23, 0.05, 0.27, 0.42])
        xBegin = -0.11
        xs = self._make_boundaries(xBegin, binWidths)
        ys = numpy.zeros(len(xs - 1))
        ws = CreateWorkspace(DataX=xs, DataY=ys)
        i = len(binWidths) / 2
        middleBinWidth = binWidths[i]
        middleBinX = xs[i] + 0.5 * middleBinWidth
        return ws, middleBinX, middleBinWidth

    def _run_algorithm(self, params):
        algorithm = testhelpers.create_algorithm('RebinToBinWidthAtX', **params)
        testhelpers.assertRaisesNothing(self, algorithm.execute)
        self.assertTrue(algorithm.isExecuted())

    def test_success_single_histogram(self):
        ws, X, expectedWidth = self._make_single_histogram_ws()
        params = self._make_algorithm_params(ws, X)
        self._run_algorithm(params)
        self._check_bin_widths(expectedWidth)
        DeleteWorkspace(ws)
        DeleteWorkspace(self._OUT_WS_NAME)

    def test_average_over_multiple_histograms(self):
        # Two histograms, center bin boundaries are aligned at -0.12.
        # X-axis widths are multiples of the final bin width so
        # rebinning results in full bins only.
        binWidths = numpy.array([0.3, 0.1, 0.8, 0.9, 0.2, 0.4])
        xs1 = self._make_boundaries(-0.42, binWidths[:3])
        xs2 = self._make_boundaries(-1.02, binWidths[3:])
        xs = numpy.concatenate((xs1, xs2))
        ys = numpy.zeros(len(xs - 2))
        ws = CreateWorkspace(DataX=xs, DataY=ys, NSpec=2)
        X = -0.1
        params = self._make_algorithm_params(ws, X)
        self._run_algorithm(params)
        expectedWidth = 0.5 * (binWidths[1] + binWidths[-2]) # Average!
        self._check_bin_widths(expectedWidth)
        DeleteWorkspace(ws)
        DeleteWorkspace(self._OUT_WS_NAME)

    def test_rounding(self):
        ws, X, unused = self._make_single_histogram_ws()
        params = self._make_algorithm_params(ws, X, '10^n')
        self._run_algorithm(params)
        expectedWidth = 0.01
        self._check_bin_widths(expectedWidth)
        DeleteWorkspace(ws)
        DeleteWorkspace(self._OUT_WS_NAME)

    def test_failure_X_out_of_bounds(self):
        ws, unused, unused = self._make_single_histogram_ws()
        X = sys.float_info.max
        params = self._make_algorithm_params(ws, X)
        algorithm = testhelpers.create_algorithm('RebinToBinWidthAtX', **params)
        self.assertRaises(RuntimeError, algorithm.execute)
        self.assertFalse(algorithm.isExecuted())
        DeleteWorkspace(ws)


if __name__ == "__main__":
    unittest.main()
