# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from copy import copy
from matplotlib.container import ErrorbarContainer
from mantid.plots.utility import MantidAxType

from unittest.mock import Mock, patch
from mantid.simpleapi import CreateWorkspace, AddTimeSeriesLog, CreateMDHistoWorkspace
from workbench.plotting.plotscriptgenerator.lines import (
    _get_plot_command_kwargs_from_line2d,
    _get_errorbar_specific_plot_kwargs,
    generate_plot_command,
    get_plot_command_kwargs,
    _get_mantid_specific_plot_kwargs,
)

from workbench.plotting.plotscriptgenerator.utils import convert_args_to_string

LINE2D_KWARGS = {
    "alpha": 0.5,
    "color": "r",
    "drawstyle": "steps",
    "fillstyle": "left",
    "label": "test label",
    "linestyle": "--",
    "linewidth": 1.1,
    "marker": "o",
    "markeredgecolor": "g",
    "markeredgewidth": 1.2,
    "markerfacecolor": "y",
    "markersize": 1.3,
    "markevery": 2,
    "visible": False,
    "zorder": 1.4,
}
ERRORBAR_ONLY_KWARGS = {"ecolor": "#ff0000", "elinewidth": 1.6, "capsize": 1.7, "capthick": 1.8, "barsabove": True}
ERRORBAR_KWARGS = copy(LINE2D_KWARGS)
ERRORBAR_KWARGS.update(ERRORBAR_ONLY_KWARGS)
MANTID_ONLY_KWARGS = {"wkspIndex": 0, "distribution": True}
MANTID_PLOTBIN_KWARGS = {"wkspIndex": 0, "distribution": True, "axis": MantidAxType.BIN}
MANTID_PLOTSPECTRUM_KWARGS = {"wkspIndex": 0, "distribution": False, "axis": MantidAxType.SPECTRUM}
ERRORBARS_HIDDEN_FUNC = "workbench.plotting.plotscriptgenerator.lines.errorbars_hidden"
GET_MANTID_PLOT_KWARGS = "workbench.plotting.plotscriptgenerator.lines._get_mantid_specific_plot_kwargs"
GET_PLOT_CMD_KWARGS_LINE2D = "workbench.plotting.plotscriptgenerator.lines._get_plot_command_kwargs_from_line2d"
GET_PLOT_CMD_KWARGS_ERRORBAR = "workbench.plotting.plotscriptgenerator.lines._get_plot_command_kwargs_from_errorbar_container"


class PlotScriptGeneratorLinesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.test_ws = CreateWorkspace(
            DataX=[10, 20, 30, 10, 20, 30], DataY=[2, 3, 4, 5], DataE=[1, 2, 3, 4], NSpec=2, OutputWorkspace="test_ws"
        )
        cls.test_mdhisto_ws = CreateMDHistoWorkspace(
            SignalInput="1,1,1",
            ErrorInput="1,1,1",
            Dimensionality=1,
            Extents="0,2",
            NumberOfBins="3",
            Names="X",
            Units="arb",
            OutputWorkspace="test_mdhisto_ws",
        )

    def setUp(self):
        fig = plt.figure()
        self.ax = fig.add_subplot(1, 1, 1, projection="mantid")

    def tearDown(self):
        plt.close()

    def test_get_plot_command_kwargs_from_line2d_returns_dict_with_correct_properties(self):
        line = self.ax.plot(self.test_ws, specNum=1, **LINE2D_KWARGS)[0]
        plot_commands_dict = _get_plot_command_kwargs_from_line2d(line)
        for key, value in LINE2D_KWARGS.items():
            self.assertEqual(value, plot_commands_dict[key])

    def test_generate_plot_command_returns_correct_string_for_line2d(self):
        kwargs = copy(LINE2D_KWARGS)
        kwargs.update(MANTID_ONLY_KWARGS)
        line = self.ax.plot(self.test_ws, **kwargs)[0]
        output = generate_plot_command(line)
        expected_command = "plot({}, {})".format(self.test_ws.name(), convert_args_to_string(None, kwargs))
        self.assertEqual(expected_command, output)

    def test_generate_plot_command_correct_arguments_for_mdhisto(self):
        kwargs = copy(LINE2D_KWARGS)
        line = self.ax.plot(self.test_mdhisto_ws, **kwargs)[0]
        output = generate_plot_command(line)
        expected_command = "plot({}, {})".format(self.test_mdhisto_ws.name(), convert_args_to_string(None, kwargs))
        self.assertEqual(expected_command, output)

    def test_generate_plot_command_returns_correct_string_for_sample_log(self):
        kwargs = copy(LINE2D_KWARGS)
        kwargs["drawstyle"] = "steps-post"
        kwargs.update({"LogName": "my_log", "ExperimentInfo": 0, "Filtered": True})
        # add a log
        AddTimeSeriesLog(self.test_ws, Name="my_log", Time="2010-01-01T00:00:00", Value=100)
        AddTimeSeriesLog(self.test_ws, Name="my_log", Time="2010-01-01T00:30:00", Value=15)
        AddTimeSeriesLog(self.test_ws, Name="my_log", Time="2010-01-01T00:50:00", Value=100.2)
        line = self.ax.plot(self.test_ws, **kwargs)[0]
        output = generate_plot_command(line)
        expected_command = "plot({}, {})".format(self.test_ws.name(), convert_args_to_string(None, kwargs))
        self.assertEqual(expected_command, output)

    def test_generate_mantid_plot_kwargs_returns_correctly_for_plot_bin(self):
        line = self.ax.plot(self.test_ws, wkspIndex=0, **LINE2D_KWARGS, axis=MantidAxType.BIN)[0]
        plot_kwargs = _get_mantid_specific_plot_kwargs(line)
        for key, value in MANTID_PLOTBIN_KWARGS.items():
            self.assertEqual(value, plot_kwargs[key])

    def test_generate_mantid_plot_kwargs_returns_correctly_for_plot_spectrum(self):
        line = self.ax.plot(self.test_ws, wkspIndex=0, **LINE2D_KWARGS, axis=MantidAxType.SPECTRUM)[0]
        plot_kwargs = _get_mantid_specific_plot_kwargs(line)
        for key, value in MANTID_PLOTSPECTRUM_KWARGS.items():
            self.assertEqual(value, plot_kwargs[key])

    def test_get_errorbar_specific_plot_kwargs_returns_dict_with_correct_properties(self):
        errorbar_kwargs = copy(ERRORBAR_KWARGS)
        errorbar_kwargs.pop("markeredgewidth")
        err_cont = self.ax.errorbar(self.test_ws, specNum=1, **errorbar_kwargs)
        plot_commands_dict = _get_errorbar_specific_plot_kwargs(err_cont)
        for key, value in ERRORBAR_ONLY_KWARGS.items():
            self.assertEqual(value, plot_commands_dict[key])

    def test_generate_plot_command_returns_correct_string_for_errorbar_container(self):
        kwargs = copy(ERRORBAR_KWARGS)
        kwargs.pop("markeredgewidth")
        kwargs.update(MANTID_ONLY_KWARGS)
        err_cont = self.ax.errorbar(self.test_ws, **kwargs)
        err_cont[2][0].set_visible(True)  # Set the errorbars to visible
        output = generate_plot_command(err_cont)
        expected_command = "errorbar({}, {})".format(self.test_ws.name(), convert_args_to_string(None, kwargs))
        self.assertEqual(expected_command, output)

    @patch(GET_MANTID_PLOT_KWARGS)
    @patch(GET_PLOT_CMD_KWARGS_ERRORBAR)
    @patch(GET_PLOT_CMD_KWARGS_LINE2D)
    def test_get_plot_command_kwargs_calls_get_plot_command_kwargs_2d_if_errorbars_hidden(
        self, mock_plot_cmd_2d, mock_plot_cmd_err, mock_mantid_spec_cmd
    ):
        mock_error_line = Mock()
        mock_artist = Mock(spec=ErrorbarContainer, __getitem__=lambda x, y: mock_error_line)
        with patch(ERRORBARS_HIDDEN_FUNC, lambda x: True):
            get_plot_command_kwargs(mock_artist)
        self.assertEqual(0, mock_plot_cmd_err.call_count)
        mock_plot_cmd_2d.assert_called_once_with(mock_error_line)

    @patch(GET_MANTID_PLOT_KWARGS)
    @patch(GET_PLOT_CMD_KWARGS_ERRORBAR)
    @patch(GET_PLOT_CMD_KWARGS_LINE2D)
    def test_get_plot_command_kwargs_calls_get_errorbar_command_kwargs_if_errorbars_visible(
        self, mock_plot_cmd_2d, mock_plot_cmd_err, mock_mantid_spec_cmd
    ):
        mock_artist = Mock(spec=ErrorbarContainer)
        with patch(ERRORBARS_HIDDEN_FUNC, lambda x: False):
            get_plot_command_kwargs(mock_artist)
        self.assertEqual(0, mock_plot_cmd_2d.call_count)
        mock_plot_cmd_err.assert_called_once_with(mock_artist)


if __name__ == "__main__":
    unittest.main()
