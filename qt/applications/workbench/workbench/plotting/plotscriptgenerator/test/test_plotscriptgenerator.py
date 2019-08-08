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
from matplotlib.axes import Axes

from mantid.plots import MantidAxes
from mantid.py3compat.mock import Mock, patch
from mantid.simpleapi import CreateWorkspace
from workbench.plotting.plotscriptgenerator import generate_script

GEN_WS_RETRIEVAL_CMDS = 'workbench.plotting.plotscriptgenerator.generate_workspace_retrieval_commands'
GEN_PLOT_CMDS = 'workbench.plotting.plotscriptgenerator.generate_plot_command'
GEN_FIGURE_CMDS = 'workbench.plotting.plotscriptgenerator.generate_figure_command'
GEN_ADD_SUBPLOT_CMDS = 'workbench.plotting.plotscriptgenerator.generate_add_subplot_command'
GEN_AXIS_LIMIT_CMDS = 'workbench.plotting.plotscriptgenerator.generate_axis_limit_commands'

SAMPLE_SCRIPT = ("from mantid.api import AnalysisDataService\n"
                 "\n"
                 "ADS.retrieve(...)\n"
                 "\n"
                 "fig = plt.figure(...)\n"
                 "ax = fig.add_subplot(...)\n"
                 "ax.plot(...)\n"
                 "ax.plot(...)\n"
                 "ax.set_xlim(...)\n"
                 "ax.set_ylim(...)\n"
                 "ax = fig.add_subplot(...)\n"
                 "ax.plot(...)\n"
                 "ax.set_xlim(...)\n"
                 "ax.set_ylim(...)\n"
                 "plt.show()")


class PlotScriptGeneratorTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.test_ws = CreateWorkspace(
            DataX=[10, 20, 30, 10, 20, 30],
            DataY=[2, 3, 4, 5],
            DataE=[1, 2, 3, 4],
            NSpec=2,
            OutputWorkspace='test_ws')

        cls.retrieval_cmds = ["from mantid.api import AnalysisDataService", "", "ADS.retrieve(...)"]
        cls.figure_cmd = "plt.figure(...)"
        cls.subplot_cmd = "add_subplot(...)"
        cls.plot_cmd = "plot(...)"
        cls.ax_limit_cmds = ['set_xlim(...)', 'set_ylim(...)']

    def _gen_mock_axes(self, **kwargs):
        mock_kwargs = {
            'get_tracked_artists': lambda: [],
            'get_lines': lambda: [Mock()],
            'legend_': False
        }
        mock_kwargs.update(kwargs)
        mock_ax = Mock(spec=MantidAxes, **mock_kwargs)
        return mock_ax

    def test_generate_script_returns_correct_number_of_lines_for_overplot(self):
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1, projection='mantid')
        ax.plot(self.test_ws, specNum=1)
        ax.errorbar(self.test_ws, specNum=2)
        output = generate_script(fig)
        num_lines = len(output.split('\n'))
        # Expect 11 lines from DEFAULT_CONTENT, 4 for workspace retrieval and 7 for plotting
        expected_num_lines = 22
        err_message = ("Expected to output {} lines, found {}.\nOutput:\n{}"
                       "".format(expected_num_lines, num_lines, output))
        plt.close()
        self.assertEqual(expected_num_lines, num_lines, msg=err_message)

    def test_generate_script_returns_None_if_no_MantidAxes_in_figure(self):
        mock_fig = Mock(get_axes=lambda: [Mock(spec=Axes)])
        self.assertEqual(None, generate_script(mock_fig))

    @patch(GEN_AXIS_LIMIT_CMDS)
    @patch(GEN_WS_RETRIEVAL_CMDS)
    @patch(GEN_PLOT_CMDS)
    @patch(GEN_FIGURE_CMDS)
    @patch(GEN_ADD_SUBPLOT_CMDS)
    def test_generate_script_compiles_script_correctly(self, mock_add_subplot_cmd, mock_figure_cmd,
                                                       mock_plot_cmd, mock_retrieval_cmd,
                                                       mock_axis_lim_cmd):
        mock_retrieval_cmd.return_value = self.retrieval_cmds
        mock_figure_cmd.return_value = self.figure_cmd
        mock_add_subplot_cmd.return_value = self.subplot_cmd
        mock_plot_cmd.return_value = self.plot_cmd
        mock_axis_lim_cmd.return_value = self.ax_limit_cmds

        mock_axes1 = self._gen_mock_axes(get_tracked_artists=lambda: [None, None])
        mock_axes2 = self._gen_mock_axes(get_tracked_artists=lambda: [None])
        mock_fig = Mock(get_axes=lambda: [mock_axes1, mock_axes2])

        output_script = generate_script(mock_fig, exclude_headers=True)
        self.assertEqual(SAMPLE_SCRIPT, output_script)

    @patch(GEN_WS_RETRIEVAL_CMDS)
    @patch(GEN_PLOT_CMDS)
    @patch(GEN_FIGURE_CMDS)
    @patch(GEN_ADD_SUBPLOT_CMDS)
    def test_generate_script_adds_legend_command_if_legend_present(
            self, mock_add_subplot_cmd, mock_figure_cmd, mock_plot_cmd, mock_retrieval_cmd):
        mock_retrieval_cmd.return_value = self.retrieval_cmds
        mock_figure_cmd.return_value = self.figure_cmd
        mock_add_subplot_cmd.return_value = self.subplot_cmd
        mock_plot_cmd.return_value = self.plot_cmd

        mock_ax = self._gen_mock_axes(legend_=True)
        mock_fig = Mock(get_axes=lambda: [mock_ax])
        self.assertIn('.legend().draggable()', generate_script(mock_fig))

    @patch(GEN_WS_RETRIEVAL_CMDS)
    @patch(GEN_PLOT_CMDS)
    @patch(GEN_FIGURE_CMDS)
    @patch(GEN_ADD_SUBPLOT_CMDS)
    def test_generate_script_does_not_add_legend_command_if_legend_present(
            self, mock_add_subplot_cmd, mock_figure_cmd, mock_plot_cmd, mock_retrieval_cmd):
        mock_retrieval_cmd.return_value = self.retrieval_cmds
        mock_figure_cmd.return_value = self.figure_cmd
        mock_add_subplot_cmd.return_value = self.subplot_cmd
        mock_plot_cmd.return_value = self.plot_cmd
        mock_fig = Mock(get_axes=lambda: [self._gen_mock_axes()])
        self.assertNotIn('.legend()', generate_script(mock_fig))


if __name__ == '__main__':
    unittest.main()
