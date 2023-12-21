# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from mantid.simpleapi import CreateEmptyTableWorkspace, CreateWorkspace, DeleteWorkspace, FindPeaksAutomatic
from mantid.api import mtd
from unittest import mock

import plugins.algorithms.WorkflowAlgorithms.FindPeaksAutomatic as _FindPeaksAutomatic


class FindPeaksAutomaticTest(unittest.TestCase):
    data_ws = None
    peak_guess_table = None
    peak_table_header = ["centre", "error centre", "height", "error height", "sigma", "error sigma", "area", "error area"]
    alg_instance = None
    x_values = None
    y_values = None

    def setUp(self):
        # Creating two peaks on an exponential background with gaussian noise
        self.x_values = np.linspace(0, 100, 1001)
        self.centre = [25, 75]
        self.height = [35, 20]
        self.width = [10, 5]
        self.y_values = self.gaussian(self.x_values, self.centre[0], self.height[0], self.width[0])

        self.y_values += self.gaussian(self.x_values, self.centre[1], self.height[1], self.width[1])

        self.background = 10 * np.ones(len(self.x_values))
        self.y_values += self.background

        # Generating a table with a guess of the position of the centre of the peaks
        peak_table = CreateEmptyTableWorkspace()
        peak_table.addColumn(type="float", name="Approximated Centre")
        peak_table.addRow([self.centre[0] + 2])
        peak_table.addRow([self.centre[1] - 3])

        self.peakids = [np.argwhere(self.x_values == self.centre[0])[0, 0], np.argwhere(self.x_values == self.centre[1])[0, 0]]

        # Generating a workspace with the data and a flat background
        self.raw_ws = CreateWorkspace(DataX=self.x_values, DataY=self.y_values, OutputWorkspace="raw_ws")
        self.data_ws = CreateWorkspace(
            DataX=np.concatenate((self.x_values, self.x_values)),
            DataY=np.concatenate((self.y_values, self.background)),
            DataE=np.sqrt(np.concatenate((self.y_values, self.background))),
            NSpec=2,
            OutputWorkspace="data_ws",
        )

        self.peak_guess_table = peak_table

        self.alg_instance = _FindPeaksAutomatic.FindPeaksAutomatic()

    def tearDown(self):
        self.delete_if_present("data_ws")
        self.delete_if_present("peak_guess_table")
        self.delete_if_present("peak_table")
        self.delete_if_present("refit_peak_table")
        self.delete_if_present("fit_cost")
        self.delete_if_present("fit_result_NormalisedCovarianceMatrix")
        self.delete_if_present("fit_result_Parameters")
        self.delete_if_present("fit_result_Workspace")
        self.delete_if_present("fit_table")
        self.delete_if_present("data_table")
        self.delete_if_present("refit_data_table")
        self.delete_if_present("tmp_table")

        self.alg_instance = None
        self.peak_guess_table = None
        self.data_ws = None

    @staticmethod
    def gaussian(xvals, centre, height, sigma):
        exponent = (xvals - centre) / (np.sqrt(2) * sigma)
        return height * np.exp(-exponent * exponent)

    @staticmethod
    def delete_if_present(workspace):
        if workspace in mtd:
            DeleteWorkspace(workspace)

    def assertTableEqual(self, expected, actual):
        self.assertEqual(expected.columnCount(), actual.columnCount())
        self.assertEqual(expected.rowCount(), actual.rowCount())
        for i in range(expected.rowCount()):
            self.assertEqual(expected.row(i), actual.row(i))

    def assertPeakFound(self, peak_params, centre, height, sigma, tolerance=0.01):
        if not np.isclose(peak_params["centre"], centre, rtol=tolerance):
            raise Exception("Expected {}, got {}. Difference greater than tolerance {}".format(centre, peak_params["centre"], tolerance))
        if not np.isclose(peak_params["height"], height, rtol=tolerance):
            raise Exception("Expected {}, got {}. Difference greater than tolerance {}".format(height, peak_params["height"], tolerance))
        if not np.isclose(peak_params["sigma"], sigma, rtol=tolerance):
            raise Exception("Expected {}, got {}. Difference greater than tolerance {}".format(sigma, peak_params["sigma"], tolerance))

    def test_algorithm_with_no_input_workspace_raises_exception(self):
        with self.assertRaises(TypeError):
            FindPeaksAutomatic()

    def test_algorithm_with_negative_acceptance_threshold_throws(self):
        with self.assertRaises(ValueError):
            FindPeaksAutomatic(InputWorkspace=self.data_ws, AcceptanceThreshold=-0.1, PlotPeaks=False)

    def test_algorithm_with_invalid_spectrum_number(self):
        # tests that a float spectrum number throws an error
        with self.assertRaises(TypeError):
            FindPeaksAutomatic(InputWorkspace=self.data_ws, PlotPeaks=False, SpectrumNumber=3.4)

        # tests that a negative integer throws an error
        with self.assertRaises(ValueError):
            FindPeaksAutomatic(InputWorkspace=self.data_ws, PlotPeaks=False, SpectrumNumber=-1)

    def test_algorithm_with_negative_smooth_window_throws(self):
        with self.assertRaises(ValueError):
            FindPeaksAutomatic(InputWorkspace=self.data_ws, SmoothWindow=-5, PlotPeaks=False)

    def test_algorithm_with_negative_num_bad_peaks_to_consider_throws(self):
        with self.assertRaises(ValueError):
            FindPeaksAutomatic(InputWorkspace=self.data_ws, BadPeaksToConsider=-3, PlotPeaks=False)

    def test_algorithm_with_negative_estimate_of_peak_sigma_throws(self):
        with self.assertRaises(ValueError):
            FindPeaksAutomatic(InputWorkspace=self.data_ws, EstimatePeakSigma=-3, PlotPeaks=False)

    def test_algorithm_with_negative_min_peak_sigma_throws(self):
        with self.assertRaises(ValueError):
            FindPeaksAutomatic(InputWorkspace=self.data_ws, MinPeakSigma=-0.1, PlotPeaks=False)

    def test_algorithm_with_negative_max_peak_sigma_throws(self):
        with self.assertRaises(ValueError):
            FindPeaksAutomatic(InputWorkspace=self.data_ws, MaxPeakSigma=-0.1, PlotPeaks=False)

    def test_algorithm_creates_all_output_workspaces(self):
        ws_name = self.raw_ws.getName()
        FindPeaksAutomatic(self.raw_ws)

        self.assertIn("{}_with_errors".format(ws_name), mtd)
        self.assertIn("{}_{}".format(self.raw_ws.getName(), "properties"), mtd)
        self.assertIn("{}_{}".format(self.raw_ws.getName(), "refit_properties"), mtd)

    def test_algorithm_works_on_specified_spectrum(self):
        x_values = np.array([np.linspace(0, 100, 1001), np.linspace(0, 100, 1001)], dtype=float)
        centre = np.array([[25, 75], [10, 60]], dtype=float)
        height = np.array([[35, 20], [40, 50]], dtype=float)
        width = np.array([[10, 5], [8, 6]], dtype=float)
        y_values = np.array(
            [
                self.gaussian(x_values[0], centre[0, 0], height[0, 0], width[0, 0]),
                self.gaussian(x_values[1], centre[1, 0], height[1, 0], width[1, 0]),
            ]
        )

        y_values += np.array(
            [
                self.gaussian(x_values[0], centre[0, 1], height[0, 1], width[0, 1]),
                self.gaussian(x_values[1], centre[1, 1], height[1, 1], width[1, 1]),
            ]
        )
        background = 10 * np.ones(x_values.shape)
        y_values += background

        raw_ws = CreateWorkspace(DataX=x_values, DataY=y_values, OutputWorkspace="raw_ws", NSpec=2)

        FindPeaksAutomatic(
            InputWorkspace=raw_ws,
            SpectrumNumber=2,
            SmoothWindow=500,
            EstimatePeakSigma=6,
            MinPeakSigma=3,
            MaxPeakSigma=15,
        )
        peak_table = mtd["{}_{}".format(raw_ws.getName(), "properties")]
        print(peak_table.row(1))
        self.assertPeakFound(peak_table.row(0), 10, 40, 8)
        self.assertPeakFound(peak_table.row(1), 60, 50, 6)

    def test_algorithm_throws_RuntimeError_when_called_with_invalid_spectrum_number(self):
        x_values = np.array([np.linspace(0, 100, 1001), np.linspace(0, 100, 1001)], dtype=float)
        centre = np.array([[25, 75], [10, 60]], dtype=float)
        height = np.array([[35, 20], [40, 50]], dtype=float)
        width = np.array([[10, 5], [8, 6]], dtype=float)
        y_values = np.array(
            [
                self.gaussian(x_values[0], centre[0, 0], height[0, 0], width[0, 0]),
                self.gaussian(x_values[1], centre[1, 0], height[1, 0], width[1, 0]),
            ]
        )

        y_values += np.array(
            [
                self.gaussian(x_values[0], centre[0, 1], height[0, 1], width[0, 1]),
                self.gaussian(x_values[1], centre[1, 1], height[1, 1], width[1, 1]),
            ]
        )
        background = 10 * np.ones(x_values.shape)
        y_values += background

        raw_ws = CreateWorkspace(DataX=x_values, DataY=y_values, OutputWorkspace="raw_ws", NSpec=2)
        with self.assertRaisesRegex(RuntimeError, "Spectrum number is not valid"):
            FindPeaksAutomatic(
                InputWorkspace=raw_ws,
                SpectrumNumber=3,
                SmoothWindow=500,
                EstimatePeakSigma=6,
                MinPeakSigma=3,
                MaxPeakSigma=15,
            )

    def test_algorithm_does_not_create_temporary_workspaces(self):
        FindPeaksAutomatic(self.raw_ws)

        self.assertNotIn("ret", mtd)
        self.assertNotIn("raw_data_ws", mtd)
        self.assertNotIn("flat_ws", mtd)
        self.assertNotIn("fit_result_NormalisedCovarianceMatrix", mtd)
        self.assertNotIn("fit_result_Parameters", mtd)
        self.assertNotIn("fit_result_Workspace", mtd)
        self.assertNotIn("fit_cost", mtd)

    def test_output_tables_are_correctly_formatted(self):
        FindPeaksAutomatic(self.raw_ws, FitToBaseline=True)

        peak_table = mtd["{}_{}".format(self.raw_ws.getName(), "properties")]
        refit_peak_table = mtd["{}_{}".format(self.raw_ws.getName(), "refit_properties")]
        self.assertEqual(self.peak_table_header, peak_table.getColumnNames())
        self.assertEqual(self.peak_table_header, refit_peak_table.getColumnNames())
        self.assertEqual(2, peak_table.rowCount())
        self.assertEqual(0, refit_peak_table.rowCount())

    def test_single_erosion_returns_correct_result(self):
        yvals = np.array([-2, 3, 1, 0, 4])

        self.assertEqual(-2, self.alg_instance._single_erosion(yvals, 2, 2))

    def test_single_erosion_checks_extremes_of_list_correctly(self):
        yvals = np.array([-5, -3, 0, 1, -2, 2, 9])

        self.assertEqual(-2, self.alg_instance._single_erosion(yvals, 3, 1))
        self.assertEqual(-3, self.alg_instance._single_erosion(yvals, 3, 2))

    def test_single_erosion_with_zero_window_does_nothing(self):
        yvals = np.array([-5, -3, 0, 1, -2, 2, 9])

        self.assertEqual(0, self.alg_instance._single_erosion(yvals, 2, 0))

    def test_single_dilation_returns_correct_result(self):
        yvals = np.array([-2, 3, 1, 0, 4])

        self.assertEqual(4, self.alg_instance._single_dilation(yvals, 2, 2))

    def test_single_dilation_checks_extremes_of_list_correctly(self):
        yvals = np.array([-5, 3, 0, -7, 2, -2, 9])

        self.assertEqual(2, self.alg_instance._single_dilation(yvals, 3, 1))
        self.assertEqual(3, self.alg_instance._single_dilation(yvals, 3, 2))

    def test_single_dilation_with_zero_window_does_nothing(self):
        yvals = np.array([-5, -3, 0, 1, -2, 2, 9])

        self.assertEqual(0, self.alg_instance._single_dilation(yvals, 2, 0))

    def test_erosion_with_zero_window_is_an_invariant(self):
        np.testing.assert_equal(self.y_values, self.alg_instance.erosion(self.y_values, 0))

    def test_erosion_calls_single_erosion_the_correct_number_of_times(
        self,
    ):
        with mock.patch(
            "plugins.algorithms.WorkflowAlgorithms.FindPeaksAutomatic.FindPeaksAutomatic._single_erosion"
        ) as mock_single_erosion:
            times = len(self.y_values)
            win_size = 2
            call_list = []
            for i in range(times):
                call_list.append(mock.call(self.y_values, i, win_size))

            self.alg_instance.erosion(self.y_values, win_size)

            self.assertEqual(times, mock_single_erosion.call_count)
            mock_single_erosion.assert_has_calls(call_list, any_order=True)

    def test_dilation_with_zero_window_is_an_invariant(self):
        np.testing.assert_equal(self.y_values, self.alg_instance.dilation(self.y_values, 0))

    def test_dilation_calls_single_erosion_the_correct_number_of_times(self):
        with mock.patch(
            "plugins.algorithms.WorkflowAlgorithms.FindPeaksAutomatic.FindPeaksAutomatic._single_dilation"
        ) as mock_single_dilation:
            times = len(self.y_values)
            win_size = 2
            call_list = []
            for i in range(times):
                call_list.append(mock.call(self.y_values, i, win_size))

            self.alg_instance.dilation(self.y_values, win_size)

            self.assertEqual(times, mock_single_dilation.call_count)
            mock_single_dilation.assert_has_calls(call_list, any_order=True)

    @mock.patch("plugins.algorithms.WorkflowAlgorithms.FindPeaksAutomatic.FindPeaksAutomatic.erosion")
    @mock.patch("plugins.algorithms.WorkflowAlgorithms.FindPeaksAutomatic.FindPeaksAutomatic.dilation")
    def test_opening_calls_correct_functions_in_correct_order(self, mock_dilation, mock_erosion):
        win_size = 3

        self.alg_instance.opening(self.y_values, win_size)
        self.assertEqual(mock_erosion.call_count, 1)
        self.assertEqual(mock_dilation.call_count, 1)

        erosion_ret = self.alg_instance.erosion(self.y_values, win_size)
        mock_erosion.assert_called_with(self.y_values, win_size)
        mock_dilation.assert_called_with(erosion_ret, win_size)

    @mock.patch("plugins.algorithms.WorkflowAlgorithms.FindPeaksAutomatic.FindPeaksAutomatic.opening")
    @mock.patch("plugins.algorithms.WorkflowAlgorithms.FindPeaksAutomatic.FindPeaksAutomatic.dilation")
    @mock.patch("plugins.algorithms.WorkflowAlgorithms.FindPeaksAutomatic.FindPeaksAutomatic.erosion")
    def test_average_calls_right_functions_in_right_order(self, mock_erosion, mock_dilation, mock_opening):
        win_size = 3

        self.alg_instance.average(self.y_values, win_size)
        self.assertEqual(mock_erosion.call_count, 1)
        self.assertEqual(mock_dilation.call_count, 1)
        self.assertEqual(mock_opening.call_count, 2)

        op_ret = self.alg_instance.opening(self.y_values, win_size)
        mock_opening.assert_called_with(self.y_values, win_size)
        mock_dilation.assert_called_with(op_ret, win_size)
        mock_erosion.assert_called_with(op_ret, win_size)

    def test_generate_peak_guess_table_correctly_formats_table(self):
        peakids = [2, 4, 10, 34]

        peak_guess_table = self.alg_instance.generate_peak_guess_table(self.x_values, peakids)

        self.assertEqual(peak_guess_table.getColumnNames(), ["centre"])

    def test_generate_peak_guess_table_with_no_peaks_generates_empty_table(self):
        peak_guess_table = self.alg_instance.generate_peak_guess_table(self.x_values, [])

        self.assertEqual(peak_guess_table.rowCount(), 0)

    def test_generate_peak_guess_table_adds_correct_values_of_peak_centre(self):
        peakids = [2, 23, 19, 34, 25, 149, 234]
        peak_guess_table = self.alg_instance.generate_peak_guess_table(self.x_values, peakids)

        for i, pid in enumerate(sorted(peakids)):
            self.assertAlmostEqual(peak_guess_table.row(i)["centre"], self.x_values[pid], 5)

    def test_find_good_peaks_calls_fit_gaussian_peaks_twice_if_no_peaks_given(self):
        with mock.patch("plugins.algorithms.WorkflowAlgorithms.FindPeaksAutomatic.FitGaussianPeaks") as mock_fit:
            tmp_table = CreateEmptyTableWorkspace()
            tmp_table.addColumn(type="float", name="chi2")
            tmp_table.addColumn(type="float", name="poisson")
            tmp_table.addRow([10, 20])
            mock_fit.return_value = (mock.MagicMock(), mock.MagicMock(), tmp_table)
            self.alg_instance.min_sigma = 1
            self.alg_instance.max_sigma = 10

            self.alg_instance.find_good_peaks(self.x_values, [], 0.1, 5, False, self.data_ws, 5)

            self.assertEqual(2, mock_fit.call_count)

    def _table_side_effect(self, idx):
        raise ValueError("Index = %d" % idx)

    def test_find_good_peaks_selects_correct_column_for_error(self):
        with mock.patch("plugins.algorithms.WorkflowAlgorithms.FindPeaksAutomatic.FitGaussianPeaks") as mock_fit:
            mock_table = mock.Mock()
            mock_table.column.side_effect = self._table_side_effect
            mock_fit.return_value = None, None, mock_table

            # chi2 cost
            with self.assertRaisesRegex(ValueError, "Index = 0"):
                self.alg_instance.find_good_peaks(self.x_values, [], 0.1, 5, False, self.data_ws, 5)

            # poisson cost
            with self.assertRaisesRegex(ValueError, "Index = 1"):
                self.alg_instance.find_good_peaks(self.x_values, [], 0.1, 5, True, self.data_ws, 5)

    def test_find_good_peaks_returns_correct_peaks(self):
        self.alg_instance._min_sigma = 1
        self.alg_instance._max_sigma = 10
        actual_peaks, peak_table, refit_peak_table = self.alg_instance.find_good_peaks(
            self.x_values, self.peakids, 0, 5, False, self.data_ws, 5
        )
        peak1 = peak_table.row(0)
        peak2 = peak_table.row(1)

        self.assertEqual(self.peakids, actual_peaks)
        self.assertEqual(0, refit_peak_table.rowCount())
        self.assertEqual(refit_peak_table.getColumnNames(), peak_table.getColumnNames())

        self.assertPeakFound(peak1, self.centre[0], self.height[0] + 10, self.width[0], 0.05)
        self.assertPeakFound(peak2, self.centre[1], self.height[1] + 10, self.width[1], 0.05)

    def test_find_peaks_is_called_correctly(self):
        mock_scipy = mock.MagicMock()
        mock_scipy.signal.find_peaks.return_value = (self.peakids, {"prominences": self.peakids})
        with mock.patch.dict("sys.modules", scipy=mock_scipy):
            self.alg_instance.process(
                self.x_values,
                self.y_values,
                raw_error=np.sqrt(self.y_values),
                acceptance=0,
                average_window=50,
                bad_peak_to_consider=2,
                use_poisson=False,
                peak_width_estimate=5,
                fit_to_baseline=False,
                prog_reporter=mock.Mock(),
            )

            self.assertEqual(2, mock_scipy.signal.find_peaks.call_count)
            self.assertEqual(0, mock_scipy.signal.find_peaks_cwt.call_count)

    def test_process_calls_find_good_peaks(self):
        with mock.patch("plugins.algorithms.WorkflowAlgorithms.FindPeaksAutomatic.CreateWorkspace") as mock_create_ws:
            mock_create_ws.return_value = self.data_ws
            self.alg_instance.find_good_peaks = mock.Mock()

            self.alg_instance.process(
                self.x_values,
                self.y_values,
                raw_error=np.sqrt(self.y_values),
                acceptance=0,
                average_window=50,
                bad_peak_to_consider=2,
                use_poisson=False,
                peak_width_estimate=5,
                fit_to_baseline=False,
                prog_reporter=mock.Mock(),
            )

            base = self.alg_instance.average(self.y_values, 50)
            base += self.alg_instance.average(self.y_values - base, 50)
            flat = self.y_values - base

            self.assertEqual(1, self.alg_instance.find_good_peaks.call_count)
            self.alg_instance.find_good_peaks.asser_called_with(
                self.x_values, flat, acceptance=0, bad_peak_to_consider=2, use_poisson=False, fit_ws=self.data_ws, peak_width_estimate=5
            )

    def test_process_returns_the_return_value_of_find_good_peaks(self):
        with mock.patch("plugins.algorithms.WorkflowAlgorithms.FindPeaksAutomatic.CreateWorkspace") as mock_create_ws:
            mock_create_ws.return_value = self.data_ws
            win_size = 500

            actual_return = self.alg_instance.process(
                self.x_values,
                self.y_values,
                raw_error=np.sqrt(self.y_values),
                acceptance=0,
                average_window=win_size,
                bad_peak_to_consider=2,
                use_poisson=False,
                peak_width_estimate=5,
                fit_to_baseline=False,
                prog_reporter=mock.Mock(),
            )
            import copy

            actual_return = copy.deepcopy(actual_return)

            base = self.alg_instance.average(self.y_values, win_size)
            base += self.alg_instance.average(self.y_values - base, win_size)
            expected_return = (
                self.alg_instance.find_good_peaks(
                    self.x_values,
                    self.peakids,
                    acceptance=0,
                    bad_peak_to_consider=2,
                    use_poisson=False,
                    fit_ws=self.data_ws,
                    peak_width_estimate=5,
                ),
                base,
            )

            self.assertEqual(expected_return[0][0], actual_return[0][0])
            self.assertTableEqual(expected_return[0][1], actual_return[0][1])
            np.testing.assert_almost_equal(expected_return[1], actual_return[1])

    def _assert_matplotlib_not_present(self, *args):
        import sys

        self.assertNotIn("matplotlib.pyplot", sys.modules)

    # If matplotlib.pyplot is imported other tests fail on windows and ubuntu
    def test_matplotlib_pyplot_is_not_imported(self):
        self.alg_instance.dilation = mock.Mock(side_effect=self._assert_matplotlib_not_present)
        self.alg_instance.opening(self.y_values, 0)

    def test_that_algorithm_finds_peaks_correctly(self):
        FindPeaksAutomatic(
            InputWorkspace=self.raw_ws,
            SmoothWindow=500,
            EstimatePeakSigma=5,
            MinPeakSigma=3,
            MaxPeakSigma=15,
        )
        peak_table = mtd["{}_{}".format(self.raw_ws.getName(), "properties")]
        refit_peak_table = mtd["{}_{}".format(self.raw_ws.getName(), "refit_properties")]

        self.assertEqual(2, peak_table.rowCount())
        self.assertEqual(0, refit_peak_table.rowCount())
        self.assertPeakFound(peak_table.row(0), self.centre[0], self.height[0], self.width[0], 0.05)
        self.assertPeakFound(peak_table.row(1), self.centre[1], self.height[1], self.width[1], 0.05)


if __name__ == "__main__":
    unittest.main()
