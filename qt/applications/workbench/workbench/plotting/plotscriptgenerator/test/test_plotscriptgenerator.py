# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

import matplotlib

matplotlib.use("Agg")  # noqa
from matplotlib.axes import Axes
from matplotlib.legend import Legend
from matplotlib.ticker import NullLocator

from mantid.plots import MantidAxes
from unittest.mock import Mock, patch
from mantid.simpleapi import CreateWorkspace
from workbench.plotting.plotscriptgenerator import generate_script

GEN_WS_RETRIEVAL_CMDS = 'workbench.plotting.plotscriptgenerator.generate_workspace_retrieval_commands'
GEN_PLOT_CMDS = 'workbench.plotting.plotscriptgenerator.generate_plot_command'
GEN_SUBPLOTS_CMD = 'workbench.plotting.plotscriptgenerator.generate_subplots_command'
GEN_AXIS_LIMIT_CMDS = 'workbench.plotting.plotscriptgenerator.generate_axis_limit_commands'
GET_AUTOSCALE_LIMITS = 'workbench.plotting.plotscriptgenerator.axes.get_autoscale_limits'
GET_FIT_COMMANDS = 'workbench.plotting.plotscriptgenerator.get_fit_cmds'

SAMPLE_SCRIPT = ("import matplotlib.pyplot as plt\n"
                 "from mantid.plots.utility import MantidAxType\n"
                 "from mantid.api import AnalysisDataService\n"
                 "\n"
                 "ADS.retrieve(...)\n"
                 "\n"
                 "fig, axes = plt.subplots(...)\n"
                 "axes[0].plot(...)\n"
                 "axes[0].plot(...)\n"
                 "axes[0].set_xlim(...)\n"
                 "axes[0].set_ylim(...)\n"
                 "\n"
                 "axes[1].plot(...)\n"
                 "axes[1].set_xlim(...)\n"
                 "axes[1].set_ylim(...)\n"
                 "\n"
                 "plt.show()"
                 "\n"
                 "# Scripting Plots in Mantid:"
                 "\n"
                 "# https://docs.mantidproject.org/tutorials/python_in_mantid/plotting/02_scripting_plots.html")

SAMPLE_SCRIPT_WITH_FIT = ("from mantid.simpleapi import Fit\n"
                          "import matplotlib.pyplot as plt\n"
                          "from mantid.plots.utility import MantidAxType\n"
                          "# Fit definition, see https://docs.mantidproject.org/algorithms/Fit-v1.html for more details\n"
                          "Function=\"GaussOsc\"\n"
                          "InputWorkspace=\"TestWorkspace\"\n"
                          "Output=\"TestOutput\"\n"
                          "Fit(Function=Function, InputWorkspace=InputWorkspace, Output=Output)\n"
                          "\n"
                          "from mantid.api import AnalysisDataService\n"
                          "\n"
                          "ADS.retrieve(...)\n"
                          "\n"
                          "fig, axes = plt.subplots(...)\n"
                          "axes[0].plot(...)\n"
                          "axes[0].plot(...)\n"
                          "axes[0].set_xlim(...)\n"
                          "axes[0].set_ylim(...)\n"
                          "\n"
                          "axes[1].plot(...)\n"
                          "axes[1].set_xlim(...)\n"
                          "axes[1].set_ylim(...)\n"
                          "\n"
                          "plt.show()"
                          "\n"
                          "# Scripting Plots in Mantid:"
                          "\n"
                          "# https://docs.mantidproject.org/tutorials/python_in_mantid/plotting/02_scripting_plots.html")


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
        cls.subplots_cmd = "plt.subplots(...)"
        cls.plot_cmd = "plot(...)"
        cls.ax_limit_cmds = ['set_xlim(...)', 'set_ylim(...)']
        cls.fit_cmds = ["Function=\"GaussOsc\"", "InputWorkspace=\"TestWorkspace\"",
                        "Output=\"TestOutput\"",
                        "Fit(Function=Function, InputWorkspace=InputWorkspace, Output=Output)"]
        cls.fit_header = ["from mantid.simpleapi import Fit"]

    def _gen_mock_axes(self, **kwargs):
        mock_kwargs = {
            'get_tracked_artists': lambda: [],
            'get_lines': lambda: [Mock()],
            'legend_': False,
            'lines': [],
            'collections': [],
            'images': [],
            'containers': [],
            'get_xlabel': lambda: '',
            'get_ylabel': lambda: '',
            'numRows': 1,
            'numCols': 1,
            'get_title': lambda: '',
            'get_xscale': lambda: 'linear',
            'get_yscale': lambda: 'linear',
            'get_xlim': lambda: (-0.02, 1.02),
            'get_ylim': lambda: (-0.02, 1.02),
            'xaxis': Mock(),
            'yaxis': Mock()
        }
        mock_kwargs.update(kwargs)
        mock_ax = Mock(spec=MantidAxes, **mock_kwargs)
        mock_ax.xaxis.minor.locator = Mock(spec=NullLocator)
        mock_ax.xaxis._gridOnMajor = False
        mock_ax.yaxis._gridOnMajor = False
        return mock_ax

    def test_generate_script_returns_None_if_no_MantidAxes_in_figure(self):
        mock_fig = Mock(get_axes=lambda: [Mock(spec=Axes)])
        self.assertEqual(None, generate_script(mock_fig))

    @patch(GET_AUTOSCALE_LIMITS)
    @patch(GEN_AXIS_LIMIT_CMDS)
    @patch(GEN_WS_RETRIEVAL_CMDS)
    @patch(GEN_PLOT_CMDS)
    @patch(GEN_SUBPLOTS_CMD)
    def test_generate_script_compiles_script_correctly(self, mock_subplots_cmd,
                                                       mock_plot_cmd, mock_retrieval_cmd,
                                                       mock_axis_lim_cmd,
                                                       mock_autoscale_lims):
        mock_retrieval_cmd.return_value = self.retrieval_cmds
        mock_subplots_cmd.return_value = self.subplots_cmd
        mock_plot_cmd.return_value = self.plot_cmd
        mock_axis_lim_cmd.return_value = self.ax_limit_cmds
        mock_autoscale_lims.return_value = (-0.02, 1.02)

        mock_axes1 = self._gen_mock_axes(get_tracked_artists=lambda: [None, None],
                                         get_lines=lambda: [None, None],
                                         numRows=1, numCols=2, colNum=0)
        mock_axes2 = self._gen_mock_axes(get_tracked_artists=lambda: [None],
                                         get_lines=lambda: [None],
                                         numRows=1, numCols=2, colNum=1)
        mock_fig = Mock(get_axes=lambda: [mock_axes1, mock_axes2],
                        axes=[mock_axes1, mock_axes2])
        mock_fig.canvas.manager.fit_browser.fit_result_ws_name = ""

        output_script = generate_script(mock_fig, exclude_headers=True)
        self.assertEqual(SAMPLE_SCRIPT, output_script)

    @patch(GET_AUTOSCALE_LIMITS)
    @patch(GEN_WS_RETRIEVAL_CMDS)
    @patch(GEN_PLOT_CMDS)
    @patch(GEN_SUBPLOTS_CMD)
    def test_generate_script_adds_legend_command_if_legend_present(
            self, mock_subplots_cmd, mock_plot_cmd, mock_retrieval_cmd,
            mock_autoscale_lims):
        mock_retrieval_cmd.return_value = self.retrieval_cmds
        mock_subplots_cmd.return_value = self.subplots_cmd
        mock_plot_cmd.return_value = self.plot_cmd
        mock_autoscale_lims.return_value = (-0.02, 1.02)

        mock_ax = self._gen_mock_axes(legend_=True)
        mock_fig = Mock(get_axes=lambda: [mock_ax])
        mock_fig.canvas.manager.fit_browser.fit_result_ws_name = ""
        if hasattr(Legend, "set_draggable"):
            self.assertIn('.legend().set_draggable(True)', generate_script(mock_fig))
        else:
            self.assertIn('.legend().draggable()', generate_script(mock_fig))

    @patch(GET_AUTOSCALE_LIMITS)
    @patch(GEN_WS_RETRIEVAL_CMDS)
    @patch(GEN_PLOT_CMDS)
    @patch(GEN_SUBPLOTS_CMD)
    def test_generate_script_does_not_add_legend_command_if_figure_has_no_legend(
            self, mock_subplots_cmd, mock_plot_cmd, mock_retrieval_cmd,
            mock_autoscale_lims):
        mock_retrieval_cmd.return_value = self.retrieval_cmds
        mock_subplots_cmd.return_value = self.subplots_cmd
        mock_plot_cmd.return_value = self.plot_cmd
        mock_autoscale_lims.return_value = (-0.02, 1.02)

        mock_fig = Mock(get_axes=lambda: [self._gen_mock_axes()])
        mock_fig.canvas.manager.fit_browser.fit_result_ws_name = ""
        self.assertNotIn('.legend()', generate_script(mock_fig))

    @patch(GET_AUTOSCALE_LIMITS)
    @patch(GEN_WS_RETRIEVAL_CMDS)
    @patch(GEN_PLOT_CMDS)
    @patch(GEN_SUBPLOTS_CMD)
    def test_generate_script_adds_minor_ticks_command_if_axes_has_minor_ticks(self,
                                                                              mock_subplots_cmd,
                                                                              mock_plot_cmd,
                                                                              mock_retrieval_cmd,
                                                                              mock_autoscale_lims):
        mock_retrieval_cmd.return_value = self.retrieval_cmds
        mock_subplots_cmd.return_value = self.subplots_cmd
        mock_plot_cmd.return_value = self.plot_cmd
        mock_autoscale_lims.return_value = (-0.02, 1.02)

        mock_ax = self._gen_mock_axes()
        mock_ax.xaxis.minor.locator = Mock()
        mock_fig = Mock(get_axes=lambda: [mock_ax])
        mock_fig.canvas.manager.fit_browser.fit_result_ws_name = ""

        self.assertIn('axes.minorticks_on()', generate_script(mock_fig))

    @patch(GET_FIT_COMMANDS)
    @patch(GET_AUTOSCALE_LIMITS)
    @patch(GEN_AXIS_LIMIT_CMDS)
    @patch(GEN_WS_RETRIEVAL_CMDS)
    @patch(GEN_PLOT_CMDS)
    @patch(GEN_SUBPLOTS_CMD)
    def test_generate_script_compiles_script_correctly_with_fit(self, mock_subplots_cmd,
                                                                mock_plot_cmd, mock_retrieval_cmd,
                                                                mock_axis_lim_cmd,
                                                                mock_autoscale_lims,
                                                                mock_fit_cmds):

        mock_retrieval_cmd.return_value = self.retrieval_cmds
        mock_subplots_cmd.return_value = self.subplots_cmd
        mock_plot_cmd.return_value = self.plot_cmd
        mock_axis_lim_cmd.return_value = self.ax_limit_cmds
        mock_autoscale_lims.return_value = (-0.02, 1.02)
        mock_fit_cmds.return_value = self.fit_cmds, self.fit_header

        mock_axes1 = self._gen_mock_axes(get_tracked_artists=lambda: [None, None],
                                         get_lines=lambda: [None, None],
                                         numRows=1, numCols=2, colNum=0)
        mock_axes2 = self._gen_mock_axes(get_tracked_artists=lambda: [None],
                                         get_lines=lambda: [None],
                                         numRows=1, numCols=2, colNum=1)
        mock_fig = Mock(get_axes=lambda: [mock_axes1, mock_axes2],
                        axes=[mock_axes1, mock_axes2])
        mock_fig.canvas.manager.fit_browser.fit_result_ws_name = "OutputWorkspace"

        output_script = generate_script(mock_fig, exclude_headers=True)
        self.assertEqual(SAMPLE_SCRIPT_WITH_FIT, output_script)

    @patch(GET_AUTOSCALE_LIMITS)
    @patch(GEN_WS_RETRIEVAL_CMDS)
    @patch(GEN_PLOT_CMDS)
    @patch(GEN_SUBPLOTS_CMD)
    def test_generate_script_adds_minor_and_major_tick_kw(self,
                                                          mock_subplots_cmd,
                                                          mock_plot_cmd,
                                                          mock_retrieval_cmd,
                                                          mock_autoscale_lims):
        mock_retrieval_cmd.return_value = self.retrieval_cmds
        mock_subplots_cmd.return_value = self.subplots_cmd
        mock_plot_cmd.return_value = self.plot_cmd
        mock_autoscale_lims.return_value = (-0.02, 1.02)

        mock_ax = self._gen_mock_axes()
        mock_ax.xaxis.minor.locator = Mock()
        mock_ax.xaxis.major.locator = Mock()
        mock_ax.xaxis.majorTicks = [None]
        mock_ax.xaxis.minorTicks = [None]

        # If this fails then the internals of matplotlib have changed, and axes.py for ticks needs to be re-thought.
        # This fails because it is an object being made to the spec defined in matplotlib so should definetely contain
        # This object, mock or not.
        self.assertTrue(hasattr(mock_ax.xaxis, "_major_tick_kw"))
        self.assertTrue(hasattr(mock_ax.xaxis, "_minor_tick_kw"))
        mock_minor_kw = "{gridOn: False, show: True, width: 6, length: 10}"
        mock_major_kw = "{gridOn: True, show: True, width: 20, length: 15}"
        mock_ax.xaxis._major_tick_kw = mock_major_kw
        mock_ax.xaxis._minor_tick_kw = mock_minor_kw

        mock_fig = Mock(get_axes=lambda: [mock_ax])
        mock_fig.canvas.manager.fit_browser.fit_result_ws_name = ""

        commands = generate_script(mock_fig)
        self.assertIn(mock_major_kw, commands)
        self.assertIn(mock_minor_kw, commands)


if __name__ == '__main__':
    unittest.main()
