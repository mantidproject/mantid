# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
# std imports
import unittest

# third party imports
import matplotlib

matplotlib.use("AGG")


# local imports
from unittest.mock import patch
from mantid.plots._compatability import plotSpectrum, plotBin
from mantid.plots.utility import MantidAxType


class compatabilityTest(unittest.TestCase):
    @patch("mantid.plots._compatability.plot")
    def test_plotSpectrum_calls_plot_with_a_string(self, plot_mock):
        ws_name = "test_plotSpectrum_calls_plot_with_a_string-1"
        wksp_indices = 0
        plotSpectrum(ws_name, wksp_indices)
        plot_mock.assert_called_once_with(
            [ws_name],
            wksp_indices=[wksp_indices],
            errors=False,
            spectrum_nums=None,
            waterfall=None,
            fig=None,
            overplot=False,
            plot_kwargs={},
        )

    @patch("mantid.plots._compatability.plot")
    def test_plotSpectrum_calls_plot_with_a_list(self, plot_mock):
        ws_name = ["test_plotSpectrum_calls_plot_with_a_list-1", "test_plotSpectrum_calls_plot_with_a_list"]
        wksp_indices = list(range(10))
        plotSpectrum(ws_name, wksp_indices)
        plot_mock.assert_called_once_with(
            ws_name, wksp_indices=wksp_indices, errors=False, spectrum_nums=None, waterfall=None, fig=None, overplot=False, plot_kwargs={}
        )

    @patch("mantid.plots._compatability.plot")
    def test_plotSpectrum_calls_plot_with_errors(self, plot_mock):
        ws_name = "test_plotSpectrum_calls_plot_with_a_string-1"
        wksp_indices = 0
        plotSpectrum(ws_name, wksp_indices, error_bars=True)
        plot_mock.assert_called_once_with(
            [ws_name],
            wksp_indices=[wksp_indices],
            errors=True,
            spectrum_nums=None,
            waterfall=None,
            fig=None,
            overplot=False,
            plot_kwargs={},
        )

    @patch("mantid.plots._compatability.plot")
    def test_plotSpectrum_calls_plot_with_spectrum_numbers(self, plot_mock):
        ws_name = "test_plotSpectrum_calls_plot_with_spectrum_numbers-1"
        wksp_indices = 0
        plotSpectrum(ws_name, spectrum_nums=wksp_indices)
        plot_mock.assert_called_once_with(
            [ws_name],
            spectrum_nums=[wksp_indices],
            errors=False,
            wksp_indices=None,
            waterfall=None,
            fig=None,
            overplot=False,
            plot_kwargs={},
        )

    @patch("mantid.plots._compatability.plot")
    def test_plotSpectrum_calls_plot_with_no_line(self, plot_mock):
        ws_name = "test_plotSpectrum_calls_plot_with_no_line-1"
        wksp_indices = list(range(10))
        plotSpectrum(ws_name, wksp_indices, type=1)
        plot_mock.assert_called_once_with(
            [ws_name],
            wksp_indices=wksp_indices,
            errors=False,
            spectrum_nums=None,
            waterfall=None,
            fig=None,
            overplot=False,
            plot_kwargs={"linestyle": "None", "marker": "."},
        )

    @patch("mantid.plots._compatability.plot")
    def test_plotSpectrum_calls_plot_with_waterfall(self, plot_mock):
        ws_name = "test_plotSpectrum_calls_plot_with_waterfall-1"
        wksp_indices = list(range(10))
        plotSpectrum(ws_name, wksp_indices, waterfall=True)
        plot_mock.assert_called_once_with(
            [ws_name], wksp_indices=wksp_indices, errors=False, spectrum_nums=None, waterfall=True, fig=None, overplot=False, plot_kwargs={}
        )

    @patch("mantid.plots._compatability.plot")
    def test_plotSpectrum_calls_plot_with_clear_window(self, plot_mock):
        ws_name = "test_plotSpectrum_calls_plot_with_clear_window-1"
        wksp_indices = list(range(10))
        fake_window = "this is a string representing a fake plotting window"
        plotSpectrum(ws_name, wksp_indices, window=fake_window, clearWindow=True)
        plot_mock.assert_called_once_with(
            [ws_name],
            wksp_indices=wksp_indices,
            errors=False,
            spectrum_nums=None,
            waterfall=None,
            fig=fake_window,
            overplot=False,
            plot_kwargs={},
        )

    @patch("mantid.plots._compatability.plot")
    def test_plotSpectrum_calls_plot_with_overplot(self, plot_mock):
        ws_name = "test_plotSpectrum_calls_plot_with_overplot-1"
        wksp_indices = list(range(10))
        fake_window = "this is a string representing a fake plotting window"
        plotSpectrum(ws_name, wksp_indices, window=fake_window)
        plot_mock.assert_called_once_with(
            [ws_name],
            wksp_indices=wksp_indices,
            errors=False,
            spectrum_nums=None,
            waterfall=None,
            fig=fake_window,
            overplot=True,
            plot_kwargs={},
        )

    @patch("mantid.plots._compatability.plot")
    def test_plotBin_calls_plot_with_a_string(self, plot_mock):
        ws_name = "test_plotBin_calls_plot_with_a_string-1"
        wksp_indices = 0
        plotBin(ws_name, wksp_indices)
        plot_mock.assert_called_once_with(
            [ws_name],
            wksp_indices=[wksp_indices],
            errors=False,
            waterfall=None,
            fig=None,
            overplot=False,
            plot_kwargs={"axis": MantidAxType.BIN},
        )

    @patch("mantid.plots._compatability.plot")
    def test_plotBin_calls_plot_with_a_list(self, plot_mock):
        ws_name = ["test_plotBin_calls_plot_with_a_list-1", "test_plotBin_calls_plot_with_a_list"]
        wksp_indices = list(range(10))
        plotBin(ws_name, wksp_indices)
        plot_mock.assert_called_once_with(
            ws_name,
            wksp_indices=wksp_indices,
            errors=False,
            waterfall=None,
            fig=None,
            overplot=False,
            plot_kwargs={"axis": MantidAxType.BIN},
        )

    @patch("mantid.plots._compatability.plot")
    def test_plotBin_calls_plot_with_errors(self, plot_mock):
        ws_name = "test_plotBin_calls_plot_with_a_string-1"
        wksp_indices = 0
        plotBin(ws_name, wksp_indices, error_bars=True)
        plot_mock.assert_called_once_with(
            [ws_name],
            wksp_indices=[wksp_indices],
            errors=True,
            waterfall=None,
            fig=None,
            overplot=False,
            plot_kwargs={"axis": MantidAxType.BIN},
        )

    @patch("mantid.plots._compatability.plot")
    def test_plotBin_calls_plot_with_no_line(self, plot_mock):
        ws_name = "test_plotBin_calls_plot_with_no_line-1"
        wksp_indices = list(range(10))
        plotBin(ws_name, wksp_indices, type=1)
        plot_mock.assert_called_once_with(
            [ws_name],
            wksp_indices=wksp_indices,
            errors=False,
            waterfall=None,
            fig=None,
            overplot=False,
            plot_kwargs={"axis": MantidAxType.BIN, "linestyle": "None", "marker": "."},
        )

    @patch("mantid.plots._compatability.plot")
    def test_plotBin_calls_plot_with_waterfall(self, plot_mock):
        ws_name = "test_plotBin_calls_plot_with_waterfall-1"
        wksp_indices = list(range(10))
        plotBin(ws_name, wksp_indices, waterfall=True)
        plot_mock.assert_called_once_with(
            [ws_name],
            wksp_indices=wksp_indices,
            errors=False,
            waterfall=True,
            fig=None,
            overplot=False,
            plot_kwargs={"axis": MantidAxType.BIN},
        )

    @patch("mantid.plots._compatability.plot")
    def test_plotBin_calls_plot_with_clear_window(self, plot_mock):
        ws_name = "test_plotBin_calls_plot_with_clear_window-1"
        wksp_indices = list(range(10))
        fake_window = "this is a string representing a fake plotting window"
        plotBin(ws_name, wksp_indices, window=fake_window, clearWindow=True)
        plot_mock.assert_called_once_with(
            [ws_name],
            wksp_indices=wksp_indices,
            errors=False,
            waterfall=None,
            fig=fake_window,
            overplot=False,
            plot_kwargs={"axis": MantidAxType.BIN},
        )

    @patch("mantid.plots._compatability.plot")
    def test_plotBin_calls_plot_with_overplot(self, plot_mock):
        ws_name = "test_plotBin_calls_plot_with_overplot-1"
        wksp_indices = list(range(10))
        fake_window = "this is a string representing a fake plotting window"
        plotBin(ws_name, wksp_indices, window=fake_window)
        plot_mock.assert_called_once_with(
            [ws_name],
            wksp_indices=wksp_indices,
            errors=False,
            waterfall=None,
            fig=fake_window,
            overplot=True,
            plot_kwargs={"axis": MantidAxType.BIN},
        )


if __name__ == "__main__":
    unittest.main()
