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
from matplotlib.axes import Axes

from mantid.plots import MantidAxes
from mantid.py3compat.mock import Mock, patch
from mantid.simpleapi import CreateWorkspace
from workbench.plotting.plotscriptgenerator import generate_script, DEFAULT_CONTENT
from workbench.plotting.plotscriptgenerator.test.test_plotscriptgeneratorlines import ERRORBAR_KWARGS

EXAMPLE_SCIRPT = ("{}"
                  "\n"
                  "from mantid.api import AnalysisDataService as ADS\n"
                  "\n"
                  "test_ws = ADS.retrieve('test_ws')\n"
                  "\n"
                  "fig = plt.figure(dpi=100.0, figsize=(6.4, 4.8), num='')\n"
                  "ax = fig.add_subplot(1, 1, 1, frame_on=True, label='', projection='mantid', "
                  "sharex=None, sharey=None, title='', visible=True, xlabel='', "
                  "xscale='linear', ylabel='', yscale='linear')\n"
                  "ax.errorbar(test_ws, alpha=0.5, barsabove=True, capsize=1.6, capthick=1.2, "
                  "color='r', distribution=False, drawstyle='steps', ecolor='#ff0000', "
                  "elinewidth=1.5, errorevery=1, fillstyle='left', label='test label', "
                  "linestyle='--', linewidth=1.1, marker='o', markeredgecolor='g', "
                  "markerfacecolor='y', markerfacecoloralt='k', markersize=1.3, markevery=2, "
                  "solid_capstyle='butt', solid_joinstyle='round', specNum=1, update_axes_labels=False, "
                  "visible=False, zorder=1.4)\n"
                  "ax.set_xlim((14.5, 25.5))\n"
                  "ax.set_ylim((0.07999999999999997, 0.52))\n"
                  "plt.show()".format(DEFAULT_CONTENT))


class PlotScriptGeneratorTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.test_ws = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30],
                                      DataY=[2, 3, 4, 5],
                                      DataE=[1, 2, 3, 4],
                                      NSpec=2,
                                      OutputWorkspace='test_ws')

    def tearDown(self):
        plt.close()

    def test_generate_script_returns_correct_string_for_single_errorbar_plot(self):
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1, projection='mantid')
        kwargs = copy(ERRORBAR_KWARGS)
        ax.errorbar(self.test_ws, **kwargs)
        self.assertEqual(EXAMPLE_SCIRPT, generate_script(fig))

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
        self.assertEqual(expected_num_lines, num_lines, msg=err_message)

    def test_generate_script_returns_None_if_no_MantidAxes_in_figure(self):
        mock_fig = Mock(get_axes=lambda: [Mock(spec=Axes)])
        self.assertEqual(None, generate_script(mock_fig))

    @patch('workbench.plotting.plotscriptgenerator.generate_workspace_retrieval_commands')
    @patch('workbench.plotting.plotscriptgenerator.generate_plot_command')
    @patch('workbench.plotting.plotscriptgenerator.generate_figure_command')
    @patch('workbench.plotting.plotscriptgenerator.generate_add_subplot_command')
    def test_generate_script_adds_legend_command_if_legend_present(self, mock_add_subplot_cmd,
                                                                   mock_figure_cmd, mock_plot_cmd,
                                                                   mock_retrieval_cmd):
        mock_add_subplot_cmd.return_value = "fig.add_subplot(...)"
        mock_figure_cmd.return_value = "plt.figure(...)"
        mock_plot_cmd.return_value = "ax.plot(...)"
        mock_retrieval_cmd.return_value = ["ADS.retrieve(...)"]
        mock_fig = Mock(
            get_axes=lambda: [Mock(spec=MantidAxes, legend_=True, get_tracked_artists=lambda: [],
                                   get_lines=lambda: [Mock(), Mock()])])
        self.assertIn('.legend().draggable()', generate_script(mock_fig))

    @patch('workbench.plotting.plotscriptgenerator.generate_workspace_retrieval_commands')
    @patch('workbench.plotting.plotscriptgenerator.generate_plot_command')
    @patch('workbench.plotting.plotscriptgenerator.generate_figure_command')
    @patch('workbench.plotting.plotscriptgenerator.generate_add_subplot_command')
    def test_generate_script_does_not_add_legend_command_if_legend_present(
            self, mock_add_subplot_cmd, mock_figure_cmd, mock_plot_cmd, mock_retrieval_cmd):
        mock_add_subplot_cmd.return_value = "fig.add_subplot(...)"
        mock_figure_cmd.return_value = "plt.figure(...)"
        mock_plot_cmd.return_value = "ax.plot(...)"
        mock_retrieval_cmd.return_value = ["ADS.retrieve(...)"]
        mock_fig = Mock(
            get_axes=lambda: [Mock(spec=MantidAxes, legend_=False, get_tracked_artists=lambda: [],
                                   get_lines=lambda: [Mock(), Mock()])])
        self.assertNotIn('.legend()', generate_script(mock_fig))


if __name__ == '__main__':
    unittest.main()
