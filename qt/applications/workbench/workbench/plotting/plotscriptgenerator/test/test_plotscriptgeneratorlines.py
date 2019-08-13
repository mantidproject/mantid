# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

import matplotlib
matplotlib.use("Agg")  # noqa
import matplotlib.pyplot as plt
from copy import copy
from matplotlib.container import ErrorbarContainer

from mantid.py3compat.mock import Mock, patch
from mantid.simpleapi import CreateWorkspace
from workbench.plotting.plotscriptgenerator.lines import (_get_plot_command_kwargs_from_line2d,
                                                          _get_errorbar_specific_plot_kwargs,
                                                          generate_plot_command,
                                                          get_plot_command_kwargs)
from workbench.plotting.plotscriptgenerator.utils import convert_args_to_string

LINE2D_KWARGS = {
    'alpha': 0.5,
    'color': 'r',
    'drawstyle': 'steps',
    'fillstyle': 'left',
    'label': 'test label',
    'linestyle': '--',
    'linewidth': 1.1,
    'marker': 'o',
    'markeredgecolor': 'g',
    'markeredgewidth': 1.2,
    'markerfacecolor': 'y',
    'markerfacecoloralt': 'k',
    'markersize': 1.3,
    'markevery': 2,
    'solid_capstyle': 'butt',
    'solid_joinstyle': 'round',
    'visible': False,
    'zorder': 1.4,
}
ERRORBAR_ONLY_KWARGS = {
    'ecolor': '#ff0000',
    'elinewidth': 1.5,
    'capsize': 1.6,
    'capthick': 1.7,
    'barsabove': True,
    'errorevery': 1
}
ERRORBAR_KWARGS = copy(LINE2D_KWARGS)
ERRORBAR_KWARGS.update(ERRORBAR_ONLY_KWARGS)
MANTID_ONLY_KWARGS = {'specNum': 1, 'distribution': False, 'update_axes_labels': False}

ERRORBARS_HIDDEN_FUNC = 'workbench.plotting.plotscriptgenerator.lines.errorbars_hidden'
GET_MANTID_PLOT_KWARGS = 'workbench.plotting.plotscriptgenerator.lines._get_mantid_specific_plot_kwargs'
GET_PLOT_CMD_KWARGS_LINE2D = 'workbench.plotting.plotscriptgenerator.lines._get_plot_command_kwargs_from_line2d'
GET_PLOT_CMD_KWARGS_ERRORBAR = ('workbench.plotting.plotscriptgenerator.lines.'
                                '_get_plot_command_kwargs_from_errorbar_container')


class PlotScriptGeneratorLinesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.test_ws = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30],
                                      DataY=[2, 3, 4, 5],
                                      DataE=[1, 2, 3, 4],
                                      NSpec=2,
                                      OutputWorkspace='test_ws')

    def setUp(self):
        fig = plt.figure()
        self.ax = fig.add_subplot(1, 1, 1, projection='mantid')

    def tearDown(self):
        plt.close()

    def test_get_plot_command_kwargs_from_line2d_returns_correct_dict(self):
        line = self.ax.plot(self.test_ws, specNum=1, **LINE2D_KWARGS)[0]
        ret = _get_plot_command_kwargs_from_line2d(line)
        self.assertEqual(LINE2D_KWARGS, ret)

    def test_generate_plot_command_returns_correct_string_for_line2d(self):
        kwargs = copy(LINE2D_KWARGS)
        kwargs.update(MANTID_ONLY_KWARGS)
        line = self.ax.plot(self.test_ws, **kwargs)[0]
        output = generate_plot_command(line)
        expected_command = ("plot({}, {})".format(self.test_ws.name(),
                                                  convert_args_to_string(None, kwargs)))
        self.assertEqual(expected_command, output)

    def test_get_errorbar_specific_plot_kwargs_returns_correct_dict(self):
        kwargs = copy(ERRORBAR_KWARGS)
        kwargs.pop('markeredgewidth')
        err_cont = self.ax.errorbar(self.test_ws, specNum=1, **kwargs)
        output = _get_errorbar_specific_plot_kwargs(err_cont)
        self.assertEqual(ERRORBAR_ONLY_KWARGS, output)

    def test_generate_plot_command_returns_correct_string_for_errorbar_container(self):
        kwargs = copy(ERRORBAR_KWARGS)
        kwargs.pop('markeredgewidth')
        kwargs.update(MANTID_ONLY_KWARGS)
        err_cont = self.ax.errorbar(self.test_ws, **kwargs)
        output = generate_plot_command(err_cont)
        expected_command = ("errorbar({}, {})".format(self.test_ws.name(),
                                                      convert_args_to_string(None, kwargs)))
        self.assertEqual(expected_command, output)

    @patch(GET_MANTID_PLOT_KWARGS)
    @patch(GET_PLOT_CMD_KWARGS_ERRORBAR)
    @patch(GET_PLOT_CMD_KWARGS_LINE2D)
    def test_get_plot_command_kwargs_calls_get_plot_command_kwargs_2d_if_errorbars_hidden(
            self, mock_plot_cmd_2d, mock_plot_cmd_err, mock_mantid_spec_cmd):
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
            self, mock_plot_cmd_2d, mock_plot_cmd_err, mock_mantid_spec_cmd):
        mock_artist = Mock(spec=ErrorbarContainer)
        with patch(ERRORBARS_HIDDEN_FUNC, lambda x: False):
            get_plot_command_kwargs(mock_artist)
        self.assertEqual(0, mock_plot_cmd_2d.call_count)
        mock_plot_cmd_err.assert_called_once_with(mock_artist)


if __name__ == '__main__':
    unittest.main()
