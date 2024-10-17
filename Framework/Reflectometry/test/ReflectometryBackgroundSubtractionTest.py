# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import mtd, WorkspaceGroup
from mantid.simpleapi import CreateWorkspace
from testhelpers import assertRaisesNothing, create_algorithm


class ReflectometryBackgroundSubtractionTest(unittest.TestCase):
    def setUp(self):
        dataX = [1, 2, 3]
        dataY = [2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 5, 5, 5, 5, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2]
        CreateWorkspace(dataX, dataY, NSpec=8, OutputWorkspace="workspace_with_peak")

    def tearDown(self):
        mtd.clear()

    def test_execution_PerDetectorAverage(self):
        """
        Check that the algorithm executes using background method PerDetectorAverage
        """
        args = {"InputWorkspace": "workspace_with_peak", "BackgroundCalculationMethod": "PerDetectorAverage", "OutputWorkspace": "output"}
        self._assert_run_algorithm_succeeds(args)

    def test_execution_Polynomial(self):
        """
        Check that the algorithm executes using background method Polynomial
        """
        args = {"InputWorkspace": "workspace_with_peak", "BackgroundCalculationMethod": "Polynomial", "OutputWorkspace": "output"}
        self._assert_run_algorithm_succeeds(args)

    def test_execution_AveragePixelFit(self):
        """
        Check that the algorithm executes using background method AveragePixelFit
        """
        args = {"InputWorkspace": "workspace_with_peak", "BackgroundCalculationMethod": "AveragePixelFit", "OutputWorkspace": "output"}
        self._assert_run_algorithm_succeeds(args)

    def test_output_PerDetectorAverage(self):
        """
        Check that the algorithm the correct output using background method AveragePixelFit
        """
        args = {
            "InputWorkspace": "workspace_with_peak",
            "ProcessingInstructions": "0-2,5-7",
            "BackgroundCalculationMethod": "PerDetectorAverage",
            "OutputWorkspace": "output",
        }
        output = self._assert_run_algorithm_succeeds(args)
        for i in range(0, output.getNumberHistograms()):
            if i == 3 or i == 4:
                for itr in range(0, output.blocksize()):
                    self.assertEqual(3.0, output.dataY(i)[itr])
            else:
                for itr in range(0, output.blocksize()):
                    self.assertEqual(0.0, output.dataY(i)[itr])

    def test_output_Polynomial(self):
        """
        Check that the algorithm the correct output using background method Polynomial
        """
        args = {
            "InputWorkspace": "workspace_with_peak",
            "ProcessingInstructions": "0-2,5-7",
            "BackgroundCalculationMethod": "Polynomial",
            "DegreeOfPolynomial": "0",
            "OutputWorkspace": "output",
        }
        output = self._assert_run_algorithm_succeeds(args)
        for i in range(0, output.getNumberHistograms()):
            if i == 3 or i == 4:
                for itr in range(0, output.blocksize()):
                    self.assertEqual(3.0, output.dataY(i)[itr])
            else:
                for itr in range(0, output.blocksize()):
                    self.assertEqual(0.0, output.dataY(i)[itr])

    def test_output_AveragePixelFit(self):
        """
        Check that the algorithm the correct output using background method AveragePixelFit
        """
        args = {
            "InputWorkspace": "workspace_with_peak",
            "ProcessingInstructions": "0-7",
            "BackgroundCalculationMethod": "AveragePixelFit",
            "PeakRange": "3-4",
            "SumPeak": False,
            "OutputWorkspace": "output",
        }
        output = self._assert_run_algorithm_succeeds(args)
        for i in range(0, output.getNumberHistograms()):
            if i == 3 or i == 4:
                for itr in range(0, output.blocksize()):
                    self.assertEqual(3.0, output.dataY(i)[itr])
            else:
                for itr in range(0, output.blocksize()):
                    self.assertEqual(0.0, output.dataY(i)[itr])

    def test_peak_range_changes_with_index_type(self):
        """
        Check that when using the background method AveragePixelFit,
        the PeakRange and ProcessingInstructions are used as index numbers
        if entered as spectrum
        """
        args = {
            "InputWorkspace": "workspace_with_peak",
            "InputWorkspaceIndexType": "SpectrumNumber",
            "ProcessingInstructions": "1-8",
            "BackgroundCalculationMethod": "AveragePixelFit",
            "PeakRange": "4-5",
            "SumPeak": False,
            "OutputWorkspace": "output",
        }
        output = self._assert_run_algorithm_succeeds(args)
        for i in range(0, output.getNumberHistograms()):
            if i == 3 or i == 4:
                for itr in range(0, output.blocksize()):
                    self.assertEqual(3.0, output.dataY(i)[itr])
            else:
                for itr in range(0, output.blocksize()):
                    self.assertEqual(0.0, output.dataY(i)[itr])

    def test_Polynomial_error_for_single_spectra(self):
        args = {"InputWorkspace": "workspace_with_peak", "ProcessingInstructions": "3", "BackgroundCalculationMethod": "Polynomial"}
        self._assert_run_algorithm_throws(args)

    def test_AveragePixelFit_error_for_single_spectra(self):
        args = {
            "InputWorkspace": "workspace_with_peak",
            "ProcessingInstructions": "3",
            "BackgroundCalculationMethod": "AveragePixelFit",
            "PeakRange": "3-4",
            "OutputWorkspace": "output",
        }
        self._assert_run_algorithm_throws(args)

    def test_AveragePixelFit_error_peakRange_outside_spectra(self):
        args = {
            "InputWorkspace": "workspace_with_peak",
            "ProcessingInstructions": "1-7",
            "BackgroundCalculationMethod": "AveragePixelFit",
            "PeakRange": "3-9",
            "OutputWorkspace": "output",
        }
        self._assert_run_algorithm_invalid_property(args)

    def test_AveragePixelFit_error_peakRange_two_ranges(self):
        args = {
            "InputWorkspace": "workspace_with_peak",
            "ProcessingInstructions": "1-7",
            "BackgroundCalculationMethod": "AveragePixelFit",
            "PeakRange": "2-4,6-7",
            "OutputWorkspace": "output",
        }
        self._assert_run_algorithm_throws(args)

    def test_validateInputs(self):
        group = WorkspaceGroup()
        mtd["group"] = group
        args = {"InputWorkspace": "group", "BackgroundCalculationMethod": "PerDetectorAverage", "OutputWorkspace": "output"}
        alg = create_algorithm("ReflectometryBackgroundSubtraction", **args)
        error_map = alg.validateInputs()
        self.assertEqual(len(error_map), 1)
        self.assertEqual(
            error_map["InputWorkspace"], "Invalid workspace type provided to IndexProperty. " "Must be convertible to MatrixWorkspace."
        )

    def _assert_run_algorithm_succeeds(self, args):
        """Run the algorithm with the given args and check it succeeds"""
        alg = create_algorithm("ReflectometryBackgroundSubtraction", **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(mtd.doesExist("output"))
        return mtd["output"]

    def _assert_run_algorithm_throws(self, args={}):
        """Run the algorithm with the given args and check it throws"""
        alg = create_algorithm("ReflectometryBackgroundSubtraction", **args)
        with self.assertRaises(RuntimeError):
            alg.execute()

    def _assert_run_algorithm_invalid_property(self, args={}):
        """Create the algorithm with the given args and check it fails"""
        with self.assertRaises(ValueError):
            create_algorithm("ReflectometryBackgroundSubtraction", **args)


if __name__ == "__main__":
    unittest.main()
