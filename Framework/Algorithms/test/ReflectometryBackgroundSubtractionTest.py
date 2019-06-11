# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import *
from testhelpers import (assertRaisesNothing, create_algorithm)

class ReflectometryBackgroundSubtraction(unittest.TestCase):

    def setUp(self):
        dataX = [1, 2, 3]
        dataY = [2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 5, 5, 5, 5, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2]
        CreateWorkspace(dataX, dataY, NSpec = 8, OutputWorkspace="workspace_with_peak")

    def tearDown(self):
        mtd.clear()

    def test_execution_PerDetectorAverage(self):
        """
            Check that the algorithm executes using background method PerDetectorAverage
        """
        args = {'InputWorkspace' : 'workspace_with_peak', 
                'BackgroundCalculationMethod' : 'PerDetectorAverage',
                'OutputWorkspace': 'output'}
        output = self._assert_run_algorithm_succeeds(args)

    def test_execution_Polynomial(self):
        """
            Check that the algorithm executes using background method Polynomial
        """
        args = {'InputWorkspace' : 'workspace_with_peak', 
                'BackgroundCalculationMethod' : 'Polynomial',
                'OutputWorkspace': 'output'}
        output = self._assert_run_algorithm_succeeds(args)

    def test_execution_AveragePixelFit(self):
        """
            Check that the algorithm executes using background method AveragePixelFit
        """
        args = {'InputWorkspace' : 'workspace_with_peak', 
                'BackgroundCalculationMethod' : 'AveragePixelFit',
                'PeakRange' : '3,4',
                'OutputWorkspace': 'output'}
        output = self._assert_run_algorithm_succeeds(args)

    def test_output_PerDetectorAverage(self):
        """
            Check that the algorithm the correct output using background method AveragePixelFit
        """
        args = {'InputWorkspace' : 'workspace_with_peak', 
                'InputWorkspaceIndexSet' : '0-2,5-7',
                'BackgroundCalculationMethod' : 'PerDetectorAverage',
                'OutputWorkspace': 'output'}
        output = self._assert_run_algorithm_succeeds(args)
        for i in range(0, output.getNumberHistograms()):
            if i == 3 or i == 4:
                for itr in range(0, output.blocksize()):
                    self.assertEquals(3.0, output.dataY(i)[itr])
            else:
                for itr in range(0, output.blocksize()):
                    self.assertEquals(0.0, output.dataY(i)[itr])

    def test_output_Polynomial(self):
        """
            Check that the algorithm the correct output using background method Polynomial
        """
        args = {'InputWorkspace' : 'workspace_with_peak', 
                'InputWorkspaceIndexSet' : '0-2,5-7',
                'BackgroundCalculationMethod' : 'Polynomial',
                'DegreeOfPolynomial' : '0',
                'OutputWorkspace': 'output'}
        output = self._assert_run_algorithm_succeeds(args)
        for i in range(0, output.getNumberHistograms()):
            if i == 3 or i == 4:
                for itr in range(0, output.blocksize()):
                    self.assertEquals(3.0, output.dataY(i)[itr])
            else:
                for itr in range(0, output.blocksize()):
                    self.assertEquals(0.0, output.dataY(i)[itr])

    def test_output_AveragePixelFit(self):
        """
            Check that the algorithm the correct output using background method AveragePixelFit
        """
        args = {'InputWorkspace' : 'workspace_with_peak', 
                'InputWorkspaceIndexSet' : '0-7',
                'BackgroundCalculationMethod' : 'AveragePixelFit',
                'PeakRange' : '3,4',
                'SumPeak' : False,
                'OutputWorkspace': 'output'}
        output = self._assert_run_algorithm_succeeds(args)
        for i in range(0, output.getNumberHistograms()):
            if i == 3 or i == 4:
                for itr in range(0, output.blocksize()):
                    self.assertEquals(3.0, output.dataY(i)[itr])
            else:
                for itr in range(0, output.blocksize()):
                    self.assertEquals(0.0, output.dataY(i)[itr])

    def test_Polynomial_error_for_single_spectra(self):
        args = {'InputWorkspace' : 'workspace_with_peak',
                'InputWorkspaceIndexSet' : '3',
                'BackgroundCalculationMethod' : 'Polynomial'}
        self._assert_run_algorithm_throws(args)

    def test_AveragePixelFit_error_for_single_spectra(self):
        args = {'InputWorkspace' : 'workspace_with_peak',
                'InputWorkspaceIndexSet' : '3',
                'BackgroundCalculationMethod' : 'AveragePixelFit',
                'PeakRange' : '3,4',
                'OutputWorkspace': 'output'}
        self._assert_run_algorithm_throws(args)

    def test_AveragePixelFit_error_peakRange_outside_spectra(self):
        args = {'InputWorkspace' : 'workspace_with_peak',
                'InputWorkspaceIndexSet' : '1-7',
                'BackgroundCalculationMethod' : 'AveragePixelFit',
                'PeakRange' : '3,9',
                'OutputWorkspace': 'output'}
        self._assert_run_algorithm_throws(args)

    def _assert_run_algorithm_succeeds(self, args):
        """ Run the algorithm with the given args and check it succeeds """
        alg = create_algorithm('ReflectometryBackgroundSubtraction', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(mtd.doesExist('output'))
        return mtd['output']

    def _assert_run_algorithm_throws(self, args = {}):
        """Run the algorithm with the given args and check it throws"""
        alg = create_algorithm('ReflectometryBackgroundSubtraction', **args)
        with self.assertRaises(RuntimeError):
            alg.execute()

if __name__ == '__main__':
    unittest.main()
