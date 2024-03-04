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
from matplotlib.axes import Axes
from matplotlib.legend import Legend
from matplotlib.ticker import NullLocator

from mantid.plots import MantidAxes
from unittest.mock import Mock, patch, MagicMock
from mantid.simpleapi import CreateWorkspace
from workbench.plotting.plotscriptgenerator import generate_script, get_legend_cmds

GEN_WS_RETRIEVAL_CMDS = "workbench.plotting.plotscriptgenerator.generate_workspace_retrieval_commands"
GEN_PLOT_CMDS = "workbench.plotting.plotscriptgenerator.generate_plot_command"
GEN_SUBPLOTS_CMD = "workbench.plotting.plotscriptgenerator.generate_subplots_command"
GEN_AXIS_LIMIT_CMDS = "workbench.plotting.plotscriptgenerator.generate_axis_limit_commands"
GET_AUTOSCALE_LIMITS = "workbench.plotting.plotscriptgenerator.axes.get_autoscale_limits"
GET_FIT_COMMANDS = "workbench.plotting.plotscriptgenerator.get_fit_cmds"

SAMPLE_SCRIPT = (
    "import matplotlib.pyplot as plt\n"
    "from mantid.plots.utility import MantidAxType\n"
    "from mantid.api import AnalysisDataService\n"
    "\n"
    "ADS.retrieve(...)\n"
    "\n"
    "fig, axes = plt.subplots(...)\n"
    "axes.plot(...)\n"
    "axes.plot(...)\n"
    "axes.set_xlim(...)\n"
    "axes.set_ylim(...)\n"
    "axes.set_facecolor('#8a9aff')\n"
    "\n"
    "fig.show()\n"
    "# Use plt.show() if running the script outside of Workbench\n"
    "#plt.show()\n"
    "# Scripting Plots in Mantid:\n"
    "# https://docs.mantidproject.org/tutorials/python_in_mantid/plotting/02_scripting_plots.html"
)

SAMPLE_SCRIPT_WITH_FIT = (
    "from mantid.simpleapi import Fit\n"
    "import matplotlib.pyplot as plt\n"
    "from mantid.plots.utility import MantidAxType\n"
    "# Fit definition, see https://docs.mantidproject.org/algorithms/Fit-v1.html for more details"
    "\n"
    'Function="GaussOsc"\n'
    'InputWorkspace="TestWorkspace"\n'
    'Output="TestOutput"\n'
    "Fit(Function=Function, InputWorkspace=InputWorkspace, Output=Output)\n"
    "\n"
    "from mantid.api import AnalysisDataService\n"
    "\n"
    "ADS.retrieve(...)\n"
    "\n"
    "fig, axes = plt.subplots(...)\n"
    "axes.plot(...)\n"
    "axes.plot(...)\n"
    "axes.set_xlim(...)\n"
    "axes.set_ylim(...)\n"
    "axes.set_facecolor('#8a9aff')\n"
    "\n"
    "fig.show()\n"
    "# Use plt.show() if running the script outside of Workbench\n"
    "#plt.show()\n"
    "# Scripting Plots in Mantid:\n"
    "# https://docs.mantidproject.org/tutorials/python_in_mantid/plotting/02_scripting_plots"
    ".html"
)

SAMPLE_SCRIPT_TILED_PLOT = (
    "import matplotlib.pyplot as plt\n"
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
    "axes[0].set_facecolor('#8a9aff')\n"
    "\n"
    "axes[1].plot(...)\n"
    "axes[1].set_xlim(...)\n"
    "axes[1].set_ylim(...)\n"
    "axes[1].set_facecolor('#8a9aff')\n"
    "\n"
    "fig.show()\n"
    "# Use plt.show() if running the script outside of Workbench\n"
    "#plt.show()\n"
    "# Scripting Plots in Mantid:\n"
    "# https://docs.mantidproject.org/tutorials/python_in_mantid/plotting/02_scripting_plots"
    ".html"
)


class PlotScriptGeneratorTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.test_ws = CreateWorkspace(
            DataX=[10, 20, 30, 10, 20, 30], DataY=[2, 3, 4, 5], DataE=[1, 2, 3, 4], NSpec=2, OutputWorkspace="test_ws"
        )

        cls.retrieval_cmds = ["from mantid.api import AnalysisDataService", "", "ADS.retrieve(...)"]
        cls.subplots_cmd = "plt.subplots(...)"
        cls.plot_cmd = "plot(...)"
        cls.ax_limit_cmds = ["set_xlim(...)", "set_ylim(...)"]
        cls.fit_cmds = [
            'Function="GaussOsc"',
            'InputWorkspace="TestWorkspace"',
            'Output="TestOutput"',
            "Fit(Function=Function, InputWorkspace=InputWorkspace, Output=Output)",
        ]
        cls.fit_header = ["from mantid.simpleapi import Fit"]

    def _gen_mock_axes(self, colNum=0, **kwargs):
        mock_kwargs = {
            "get_tracked_artists": lambda: [],
            "get_lines": lambda: [Mock()],
            "get_facecolor": lambda: "#8a9aff",
            "legend_": False,
            "lines": [],
            "collections": [],
            "images": [],
            "containers": [],
            "get_xlabel": lambda: "",
            "get_ylabel": lambda: "",
            "get_title": lambda: "",
            "get_xscale": lambda: "linear",
            "get_yscale": lambda: "linear",
            "get_xlim": lambda: (-0.02, 1.02),
            "get_ylim": lambda: (-0.02, 1.02),
            "xaxis": Mock(),
            "yaxis": Mock(),
        }
        mock_kwargs.update(kwargs)
        mock_ax = MagicMock(spec=MantidAxes, **mock_kwargs)
        num_rows = kwargs.get("numRows", 1)
        num_cols = kwargs.get("numCols", 1)
        setattr(mock_ax, "get_gridspec", MagicMock())
        mock_ax.get_gridspec.return_value.nrows = num_rows
        mock_ax.get_gridspec.return_value.ncols = num_cols
        setattr(mock_ax, "get_subplotspec", MagicMock())
        mock_ax.get_subplotspec.return_value.colspan.start = colNum

        mock_ax.xaxis.minor.locator = Mock(spec=NullLocator)
        mock_ax.xaxis._major_tick_kw = {"gridOn": False}
        mock_ax.yaxis._major_tick_kw = {"gridOn": False}
        return mock_ax

    def test_generate_script_returns_None_if_no_MantidAxes_in_figure(self):
        mock_fig = Mock(get_axes=lambda: [Mock(spec=Axes)])
        self.assertEqual(None, generate_script(mock_fig))

    @patch(GET_AUTOSCALE_LIMITS)
    @patch(GEN_AXIS_LIMIT_CMDS)
    @patch(GEN_WS_RETRIEVAL_CMDS)
    @patch(GEN_PLOT_CMDS)
    @patch(GEN_SUBPLOTS_CMD)
    def test_generate_script_compiles_script_correctly(
        self, mock_subplots_cmd, mock_plot_cmd, mock_retrieval_cmd, mock_axis_lim_cmd, mock_autoscale_lims
    ):
        mock_retrieval_cmd.return_value = self.retrieval_cmds
        mock_subplots_cmd.return_value = self.subplots_cmd
        mock_plot_cmd.return_value = self.plot_cmd
        mock_axis_lim_cmd.return_value = self.ax_limit_cmds
        mock_autoscale_lims.return_value = (-0.02, 1.02)

        mock_axes1 = self._gen_mock_axes(
            get_tracked_artists=lambda: [None, None], get_lines=lambda: [None, None], numRows=1, numCols=1, colNum=0
        )
        mock_fig = Mock(get_axes=lambda: [mock_axes1], axes=[mock_axes1])
        mock_fig.canvas.manager.fit_browser.fit_result_ws_name = ""

        output_script = generate_script(mock_fig, exclude_headers=True)
        self.assertEqual(SAMPLE_SCRIPT, output_script)

    def test_generate_script_adds_legend_commands_if_legend_present(self):
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1, projection="mantid")
        ax.plot(self.test_ws, wkspIndex=0)
        ax.plot(self.test_ws, wkspIndex=1)
        ax.legend()
        legend_commands = get_legend_cmds(ax, "axes")
        # Should be some legend commands.
        self.assertTrue(len(legend_commands) > 0)

        if hasattr(Legend, "set_draggable"):
            self.assertIn(".legend().set_draggable(True)", legend_commands[0])
        else:
            self.assertIn(".legend().draggable()", legend_commands[0])
        plt.close()

    def test_generate_script_does_not_add_legend_commands_if_figure_has_no_legend(self):
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1, projection="mantid")
        ax.plot(self.test_ws, wkspIndex=0)
        ax.plot(self.test_ws, wkspIndex=1)
        # Attempt to generate legend commands when no legend is present.
        legend_commands = get_legend_cmds(ax, "axes")
        # Should be no legend commands.
        self.assertEqual(0, len(legend_commands))
        plt.close()

    @patch(GET_AUTOSCALE_LIMITS)
    @patch(GEN_WS_RETRIEVAL_CMDS)
    @patch(GEN_PLOT_CMDS)
    @patch(GEN_SUBPLOTS_CMD)
    def test_generate_script_adds_minor_ticks_command_if_axes_has_minor_ticks(
        self, mock_subplots_cmd, mock_plot_cmd, mock_retrieval_cmd, mock_autoscale_lims
    ):
        mock_retrieval_cmd.return_value = self.retrieval_cmds
        mock_subplots_cmd.return_value = self.subplots_cmd
        mock_plot_cmd.return_value = self.plot_cmd
        mock_autoscale_lims.return_value = (-0.02, 1.02)

        mock_ax = self._gen_mock_axes()
        mock_ax.xaxis.minor.locator = Mock()
        mock_ax.numRows = 1
        mock_ax.numCols = 1
        mock_fig = Mock(get_axes=lambda: [mock_ax])
        mock_fig.canvas.manager.fit_browser.fit_result_ws_name = ""

        self.assertIn("axes.minorticks_on()", generate_script(mock_fig))

    @patch(GET_FIT_COMMANDS)
    @patch(GET_AUTOSCALE_LIMITS)
    @patch(GEN_AXIS_LIMIT_CMDS)
    @patch(GEN_WS_RETRIEVAL_CMDS)
    @patch(GEN_PLOT_CMDS)
    @patch(GEN_SUBPLOTS_CMD)
    def test_generate_script_compiles_script_correctly_with_fit(
        self, mock_subplots_cmd, mock_plot_cmd, mock_retrieval_cmd, mock_axis_lim_cmd, mock_autoscale_lims, mock_fit_cmds
    ):
        mock_retrieval_cmd.return_value = self.retrieval_cmds
        mock_subplots_cmd.return_value = self.subplots_cmd
        mock_plot_cmd.return_value = self.plot_cmd
        mock_axis_lim_cmd.return_value = self.ax_limit_cmds
        mock_autoscale_lims.return_value = (-0.02, 1.02)
        mock_fit_cmds.return_value = self.fit_cmds, self.fit_header

        mock_axes1 = self._gen_mock_axes(
            get_tracked_artists=lambda: [None, None], get_lines=lambda: [None, None], numRows=1, numCols=1, colNum=0
        )
        mock_fig = Mock(get_axes=lambda: [mock_axes1], axes=[mock_axes1])
        mock_fig.canvas.manager.fit_browser.fit_result_ws_name = "OutputWorkspace"

        output_script = generate_script(mock_fig, exclude_headers=True)
        self.assertEqual(SAMPLE_SCRIPT_WITH_FIT, output_script)

    @patch(GET_AUTOSCALE_LIMITS)
    @patch(GEN_WS_RETRIEVAL_CMDS)
    @patch(GEN_PLOT_CMDS)
    @patch(GEN_SUBPLOTS_CMD)
    def test_generate_script_adds_minor_and_major_tick_kw(self, mock_subplots_cmd, mock_plot_cmd, mock_retrieval_cmd, mock_autoscale_lims):
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
        # in the old version it passes a string to a attribute that should be a dict breaking the code
        mock_minor_kw = {"gridOn": False, "show": True, "width": 6, "length": 10}
        mock_major_kw = {"gridOn": True, "show": True, "width": 20, "length": 15}
        mock_ax.xaxis._major_tick_kw = mock_major_kw
        mock_ax.xaxis._minor_tick_kw = mock_minor_kw
        # for assert test
        mock_minor_kw_str = str(mock_minor_kw)
        mock_major_kw_str = str(mock_major_kw)
        mock_ax.numRows = 1
        mock_ax.numCols = 1

        mock_fig = Mock(get_axes=lambda: [mock_ax])
        mock_fig.canvas.manager.fit_browser.fit_result_ws_name = ""

        commands = generate_script(mock_fig)
        self.assertIn(mock_major_kw_str, commands)
        self.assertIn(mock_minor_kw_str, commands)

    @patch(GET_AUTOSCALE_LIMITS)
    @patch(GEN_AXIS_LIMIT_CMDS)
    @patch(GEN_WS_RETRIEVAL_CMDS)
    @patch(GEN_PLOT_CMDS)
    @patch(GEN_SUBPLOTS_CMD)
    def test_generate_script_compiles_script_correctly_for_tiled_plots(
        self, mock_subplots_cmd, mock_plot_cmd, mock_retrieval_cmd, mock_axis_lim_cmd, mock_autoscale_lims
    ):
        mock_retrieval_cmd.return_value = self.retrieval_cmds
        mock_subplots_cmd.return_value = self.subplots_cmd
        mock_plot_cmd.return_value = self.plot_cmd
        mock_axis_lim_cmd.return_value = self.ax_limit_cmds
        mock_autoscale_lims.return_value = (-0.02, 1.02)

        mock_axes1 = self._gen_mock_axes(
            get_tracked_artists=lambda: [None, None], get_lines=lambda: [None, None], numRows=1, numCols=2, colNum=0
        )
        mock_axes2 = self._gen_mock_axes(get_tracked_artists=lambda: [None], get_lines=lambda: [None], numRows=1, numCols=2, colNum=1)
        mock_fig = Mock(get_axes=lambda: [mock_axes1, mock_axes2], axes=[mock_axes1, mock_axes2])
        mock_fig.canvas.manager.fit_browser.fit_result_ws_name = ""

        output_script = generate_script(mock_fig, exclude_headers=True)
        self.assertEqual(SAMPLE_SCRIPT_TILED_PLOT, output_script)


if __name__ == "__main__":
    unittest.main()
