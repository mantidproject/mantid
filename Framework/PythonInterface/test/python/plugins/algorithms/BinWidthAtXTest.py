# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateWorkspace, DeleteWorkspace
import numpy
import testhelpers
import unittest


class BinWidthAtXTest(unittest.TestCase):
    def _make_algorithm_params(self, ws, x, rounding="None"):
        return {"InputWorkspace": ws, "X": x, "rethrow": True, "Rounding": rounding}  # Let exceptions through for testing.

    def _make_boundaries(self, xBegin, binWidths):
        return numpy.cumsum(numpy.append(numpy.array([xBegin]), binWidths))

    def _make_single_histogram_ws(self):
        # X-axis width is a multiple of the final bin width so rebinning
        # creates full bins only.
        binWidths = numpy.array([0.13, 0.23, 0.05, 0.27, 0.42])
        xBegin = -0.11
        xs = self._make_boundaries(xBegin, binWidths)
        ys = numpy.zeros(len(xs) - 1)
        ws = CreateWorkspace(DataX=xs, DataY=ys)
        i = len(binWidths) // 2
        middleBinWidth = binWidths[i]
        middleBinX = xs[i] + 0.5 * middleBinWidth
        return ws, middleBinX, middleBinWidth

    def _run_algorithm(self, params):
        algorithm = testhelpers.create_algorithm("BinWidthAtX", **params)
        testhelpers.assertRaisesNothing(self, algorithm.execute)
        self.assertTrue(algorithm.isExecuted())
        return algorithm.getProperty("BinWidth").value

    def test_success_single_histogram(self):
        ws, X, expectedWidth = self._make_single_histogram_ws()
        params = self._make_algorithm_params(ws, X)
        binWidth = self._run_algorithm(params)
        self.assertAlmostEqual(binWidth, expectedWidth)
        DeleteWorkspace(ws)

    def test_average_over_multiple_histograms(self):
        # Two histograms, center bin boundaries are aligned at -0.12.
        # X-axis widths are multiples of the final bin width so
        # rebinning results in full bins only.
        binWidths = numpy.array([0.3, 0.1, 0.8, 0.9, 0.2, 0.4])
        xs1 = self._make_boundaries(-0.42, binWidths[:3])
        xs2 = self._make_boundaries(-1.02, binWidths[3:])
        xs = numpy.concatenate((xs1, xs2))
        ys = numpy.zeros(len(xs) - 2)
        ws = CreateWorkspace(DataX=xs, DataY=ys, NSpec=2)
        X = -0.1
        params = self._make_algorithm_params(ws, X)
        binWidth = self._run_algorithm(params)
        expectedWidth = 0.5 * (binWidths[1] + binWidths[-2])  # Average!
        self.assertAlmostEqual(binWidth, expectedWidth)
        DeleteWorkspace(ws)

    def test_rounding(self):
        ws, X, unused = self._make_single_histogram_ws()
        params = self._make_algorithm_params(ws, X, "10^n")
        binWidth = self._run_algorithm(params)
        expectedWidth = 0.01
        self.assertAlmostEqual(binWidth, expectedWidth)
        DeleteWorkspace(ws)

    def test_failure_X_out_of_bounds(self):
        ws, unused, unused = self._make_single_histogram_ws()
        X = 10000.0
        params = self._make_algorithm_params(ws, X)
        algorithm = testhelpers.create_algorithm("BinWidthAtX", **params)
        self.assertRaisesRegex(RuntimeError, f"X = {X} out of range for workspace index 0", algorithm.execute)
        self.assertFalse(algorithm.isExecuted())
        DeleteWorkspace(ws)

    def test_throws_on_non_histogram_input(self):
        xs = numpy.array([-2.2, -1.2, 0.1, 0.9])
        ys = numpy.zeros(len(xs))
        ws = CreateWorkspace(DataX=xs, DataY=ys)
        X = -0.3
        params = self._make_algorithm_params(ws, X)
        self.assertRaisesRegex(
            ValueError, "The workspace must contain histogram data", testhelpers.create_algorithm, "BinWidthAtX", **params
        )
        DeleteWorkspace(ws)

    def test_positive_output_even_if_descending_x(self):
        xs = numpy.array([110.0, 60.0, 40.0, -10.0])
        ys = numpy.zeros(len(xs) - 1)
        ws = CreateWorkspace(DataX=xs, DataY=ys)
        params = self._make_algorithm_params(ws, 50.0)
        binWidth = self._run_algorithm(params)
        expectedBinWidth = 20.0
        self.assertAlmostEqual(binWidth, expectedBinWidth)
        DeleteWorkspace(ws)


if __name__ == "__main__":
    unittest.main()
