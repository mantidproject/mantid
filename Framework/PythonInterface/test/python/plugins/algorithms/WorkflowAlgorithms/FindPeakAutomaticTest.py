# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import numpy as np

from mantid.simpleapi import CreateEmptyTableWorkspace, CreateWorkspace, DeleteWorkspace, FindPeakAutomatic
from mantid.api import mtd
from mantid.py3compat import mock

import plugins.algorithms.WorkflowAlgorithms.FindPeakAutomatic as _FindPeakAutomatic


class FindPeakAutomaticTest(unittest.TestCase):
    data_ws = None
    peak_guess_table = None
    peak_table_header = ['centre', 'error centre', 'height', 'error height',
                         'sigma', 'error sigma', 'area', 'error area']
    alg_instance = None
    x_values = None
    y_values = None

    def setUp(self):
        # Creating two peaks on an exponential background with gaussian noise
        self.x_values = np.linspace(0, 100, 1001)
        centre = [25, 75]
        height = [35, 20]
        width = [10, 5]
        self.y_values = self._gaussian_peak(self.x_values, centre[0], height[0], width[0])
        self.y_values += self._gaussian_peak(self.x_values, centre[1], height[1], width[1])
        background = 10 * np.ones(len(self.x_values))
        self.y_values += background

        # Generating a table with a guess of the position of the centre of the peaks
        peak_table = CreateEmptyTableWorkspace()
        peak_table.addColumn(type='float', name='Approximated Centre')
        peak_table.addRow([centre[0] + 2])
        peak_table.addRow([centre[1] - 3])

        # Generating a workspace with the data and a flat background
        data_ws = CreateWorkspace(DataX=np.concatenate((self.x_values, self.x_values)),
                                  DataY=np.concatenate((self.y_values, background)),
                                  DataE=np.sqrt(np.concatenate((self.y_values, background))),
                                  NSpec=2)

        self.data_ws = data_ws
        self.peak_guess_table = peak_table

        self.alg_instance = _FindPeakAutomatic.FindPeakAutomatic()

    def tearDown(self):
        self.delete_if_present('data_ws')
        self.delete_if_present('peak_guess_table')
        self.delete_if_present('peak_table')
        self.delete_if_present('refit_peak_table')
        self.delete_if_present('fit_cost')
        self.delete_if_present('fit_result_NormalisedCovarianceMatrix')
        self.delete_if_present('fit_result_Parameters')
        self.delete_if_present('fit_result_Workspace')
        self.delete_if_present('fit_table')
        self.delete_if_present('data_table')
        self.delete_if_present('refit_data_table')

        self.alg_instance = None
        self.peak_guess_table = None
        self.data_ws = None

    @staticmethod
    def _gaussian_peak(xvals, centre, height, sigma):
        exponent = (xvals - centre) / (np.sqrt(2) * sigma)
        return height * np.exp(-exponent*exponent)

    @staticmethod
    def delete_if_present(workspace):
        if workspace in mtd:
            DeleteWorkspace(workspace)

    def test_algorithm_with_no_input_workspace_raises_exception(self):
        with self.assertRaises(RuntimeError):
            FindPeakAutomatic()

    def test_algorithm_with_negative_acceptance_threshold_throws(self):
        with self.assertRaises(ValueError):
            FindPeakAutomatic(InputWorkspace=self.data_ws,
                              AcceptanceThreshold=-0.1,
                              PlotPeaks=False)

    def test_algorithm_with_negative_smooth_window_throws(self):
        with self.assertRaises(ValueError):
            FindPeakAutomatic(InputWorkspace=self.data_ws,
                              SmoothWindow=-5,
                              PlotPeaks=False)

    def test_algorithm_with_negative_num_bad_peaks_to_consider_throws(self):
        with self.assertRaises(ValueError):
            FindPeakAutomatic(InputWorkspace=self.data_ws,
                              BadPeaksToConsider=-3,
                              PlotPeaks=False)

    def test_algorithm_with_negative_estimate_of_peak_sigma_throws(self):
        with self.assertRaises(ValueError):
            FindPeakAutomatic(InputWorkspace=self.data_ws,
                              EstimatePeakSigma=-3,
                              PlotPeaks=False)

    def test_algorithm_with_negative_min_peak_sigma_throws(self):
        with self.assertRaises(ValueError):
            FindPeakAutomatic(InputWorkspace=self.data_ws,
                              MinPeakSigma=-0.1,
                              PlotPeaks=False)

    def test_algorithm_with_negative_max_peak_sigma_throws(self):
        with self.assertRaises(ValueError):
            FindPeakAutomatic(InputWorkspace=self.data_ws,
                              MaxPeakSigma=-0.1,
                              PlotPeaks=False)

    def test_that_single_erosion_returns_correct_result(self):
        yvals = np.array([-2, 3, 1, 0, 4])

        self.assertEqual(-2, self.alg_instance._single_erosion(yvals, 2, 2))

    def test_that_single_erosion_checks_extremes_of_list_correctly(self):
        yvals = np.array([-5, -3, 0, 1, -2, 2, 9])

        self.assertEqual(-2, self.alg_instance._single_erosion(yvals, 3, 1))
        self.assertEqual(-3, self.alg_instance._single_erosion(yvals, 3, 2))

    def test_that_single_erosion_with_zero_window_does_nothing(self):
        yvals = np.array([-5, -3, 0, 1, -2, 2, 9])

        self.assertEqual(0, self.alg_instance._single_erosion(yvals, 2, 0))

    def test_that_single_dilation_returns_correct_result(self):
        yvals = np.array([-2, 3, 1, 0, 4])

        self.assertEqual(4, self.alg_instance._single_dilation(yvals, 2, 2))

    def test_that_single_dilation_checks_extremes_of_list_correctly(self):
        yvals = np.array([-5, 3, 0, -7, 2, -2, 9])

        self.assertEqual(2, self.alg_instance._single_dilation(yvals, 3, 1))
        self.assertEqual(3, self.alg_instance._single_dilation(yvals, 3, 2))

    def test_that_single_dilation_with_zero_window_does_nothing(self):
        yvals = np.array([-5, -3, 0, 1, -2, 2, 9])

        self.assertEqual(0, self.alg_instance._single_dilation(yvals, 2, 0))

    def test_that_erosion_with_zero_window_is_an_invariant(self):
        np.testing.assert_equal(self.y_values, self.alg_instance.erosion(self.y_values, 0))

    @mock.patch('plugins.algorithms.WorkflowAlgorithms.FindPeakAutomatic.FindPeakAutomatic._single_erosion')
    def test_that_erosion_calls_single_erosion_the_correct_number_of_times(self, mock_single_erosion):
        times = len(self.y_values)
        win_size = 2
        call_list = []
        for i in range(times):
            call_list.append(mock.call(self.y_values, i, win_size))

        self.alg_instance.erosion(self.y_values, win_size)

        self.assertEqual(times, mock_single_erosion.call_count)
        mock_single_erosion.assert_has_calls(call_list, any_order=True)

    def test_that_dilation_with_zero_window_is_an_invariant(self):
        np.testing.assert_equal(self.y_values, self.alg_instance.dilation(self.y_values, 0))

    @mock.patch('plugins.algorithms.WorkflowAlgorithms.FindPeakAutomatic.FindPeakAutomatic._single_dilation')
    def test_that_dilation_calls_single_erosion_the_correct_number_of_times(self, mock_single_dilation):
        times = len(self.y_values)
        win_size = 2
        call_list = []
        for i in range(times):
            call_list.append(mock.call(self.y_values, i, win_size))

        self.alg_instance.dilation(self.y_values, win_size)

        self.assertEqual(times, mock_single_dilation.call_count)
        mock_single_dilation.assert_has_calls(call_list, any_order=True)

    @mock.patch('plugins.algorithms.WorkflowAlgorithms.FindPeakAutomatic.FindPeakAutomatic.erosion')
    @mock.patch('plugins.algorithms.WorkflowAlgorithms.FindPeakAutomatic.FindPeakAutomatic.dilation')
    def test_that_opening_calls_correct_functions_in_correct_order(self, mock_dilation, mock_erosion):
        win_size = 3

        self.alg_instance.opening(self.y_values, win_size)
        self.assertEqual(mock_erosion.call_count, 1)
        self.assertEqual(mock_dilation.call_count, 1)

        erosion_ret = self.alg_instance.erosion(self.y_values, win_size)
        mock_erosion.assert_called_with(self.y_values, win_size)
        mock_dilation.assert_called_with(erosion_ret, win_size)

    @mock.patch('plugins.algorithms.WorkflowAlgorithms.FindPeakAutomatic.FindPeakAutomatic.opening')
    @mock.patch('plugins.algorithms.WorkflowAlgorithms.FindPeakAutomatic.FindPeakAutomatic.dilation')
    @mock.patch('plugins.algorithms.WorkflowAlgorithms.FindPeakAutomatic.FindPeakAutomatic.erosion')
    def test_that_average_calls_right_functions_in_right_order(self, mock_erosion, mock_dilation, mock_opening):
        win_size = 3

        self.alg_instance.average(self.y_values, win_size)
        self.assertEqual(mock_erosion.call_count, 1)
        self.assertEqual(mock_dilation.call_count, 1)
        self.assertEqual(mock_opening.call_count, 2)

        op_ret = self.alg_instance.opening(self.y_values, win_size)
        mock_opening.assert_called_with(self.y_values, win_size)
        mock_dilation.assert_called_with(op_ret, win_size)
        mock_erosion.assert_called_with(op_ret, win_size)

    def test_that_generate_peak_guess_table_correctly_formats_table(self):
        peakids = [2, 4, 10, 34]

        peak_guess_table = self.alg_instance.generate_peak_guess_table(self.x_values, peakids)

        self.assertEqual(peak_guess_table.getColumnNames(), ['centre'])

    def test_that_generate_peak_guess_table_with_no_peaks_generates_empty_table(self):
        peak_guess_table = self.alg_instance.generate_peak_guess_table(self.x_values, [])

        self.assertEqual(peak_guess_table.rowCount(), 0)

    def test_that_generate_peak_guess_table_adds_correct_values_of_peak_centre(self):
        peakids = [2, 23, 19, 34, 25, 149, 234]
        peak_guess_table = self.alg_instance.generate_peak_guess_table(self.x_values, peakids)

        for i, pid in enumerate(sorted(peakids)):
            self.assertAlmostEqual(peak_guess_table.row(i)['centre'], self.x_values[pid], 5)


if __name__ == '__main__':
    unittest.main()