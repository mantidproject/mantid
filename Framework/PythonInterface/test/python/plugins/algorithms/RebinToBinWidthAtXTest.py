from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import CreateWorkspace
import numpy
import testhelpers
import unittest

class RebinToBinWidthAtXTest(unittest.TestCase):

    def test_success_single_histogram(self):
        # X-axis width is a multiple of the final bin width so rebinning
        # creates full bins only.
        binWidths = numpy.array([0.13, 0.23, 0.05, 0.27, 0.42])
        xBegin = -0.11
        xs = numpy.cumsum(numpy.append(numpy.array([xBegin]), binWidths))
        ys = numpy.zeros(len(xs - 1))
        ws = CreateWorkspace(DataX=xs, DataY=ys)
        i = len(xs) / 2 - 1
        X = xs[i] + 0.5 * binWidths[i]
        params = {
            'InputWorkspace': ws,
            'OutputWorkspace': 'outWs',
            'X': X,
            'Rounding': 'None'
        }
        algorithm = testhelpers.run_algorithm('RebinToBinWidthAtX', **params)
        self.assertTrue(algorithm.isExecuted())
        expectedWidth = binWidths[i]
        binnedXs = mtd['outWs'].readX(0)
        newBins = binnedXs[1:] - binnedXs[:-1]
        for binWidth in newBins:
            self.assertAlmostEqual(binWidth, expectedWidth)

    def test_average_over_multiple_histograms(self):
        # Two histograms, center bin boundaries are aligned at -0.12.
        # X-axis widths are multiples of the final bin width so
        # rebinning results in full bins only.
        binWidths = numpy.array([0.3, 0.1, 0.8, 0.9, 0.2, 0.4])
        xs1 = numpy.cumsum(numpy.append(numpy.array([-0.42]), binWidths[:3]))
        xs2 = numpy.cumsum(numpy.append(numpy.array([-1.02]), binWidths[3:]))
        xs = numpy.concatenate((xs1, xs2))
        ys = numpy.zeros(len(xs - 2))
        ws = CreateWorkspace(DataX=xs, DataY=ys, NSpec=2)
        X = -0.1
        params = {
            'InputWorkspace': ws,
            'OutputWorkspace': 'outWs',
            'X': X,
            'Rounding': 'None'
        }
        algorithm = testhelpers.run_algorithm('RebinToBinWidthAtX', **params)
        self.assertTrue(algorithm.isExecuted())
        expectedWidth = 0.5 * (binWidths[1] + binWidths[-2]) # Average!
        outWs = mtd['outWs']
        for i in range(outWs.getNumberHistograms()):
            binnedXs = outWs.readX(0)
            newBins = binnedXs[1:] - binnedXs[:-1]
            for binWidth in newBins:
                self.assertAlmostEqual(binWidth, expectedWidth)

    def test_rounding(self):
        pass

    def test_failure_X_out_of_bounds(self):
        pass

if __name__ == "__main__":
    unittest.main()
