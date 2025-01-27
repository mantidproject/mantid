# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest.mock import Mock, MagicMock, ANY

from matplotlib.lines import Line2D

from mantid.plots import MantidAxes
from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.widgets.fitpropertybrowser import FitPropertyBrowser
from mantidqt.widgets.fitpropertybrowser.fitpropertybrowserplotinteraction import FitPropertyBrowserPlotInteraction
from mantid.api import AnalysisDataService, FunctionFactory, WorkspaceFactory

import matplotlib

matplotlib.use("AGG")

X_COLUMN_LABEL = "x_column"
Y_COLUMN_LABEL = "y_column"
FULL_FUNCTION = FunctionFactory.createInitialized(
    "name=FlatBackground,A0=1;name=LinearBackground,A0=1,A1=2;name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0"
)
FUNCTION_1 = FunctionFactory.createInitialized("name=FlatBackground,A0=1")
FUNCTION_2 = FunctionFactory.createInitialized("name=LinearBackground,A0=1,A1=2")
FUNCTION_3 = FunctionFactory.createInitialized("name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0")


class FitPropertyBrowserPlotInteractionTest(unittest.TestCase):
    def setup_mock_fit_browser(self, workspace_creator, workspace_name, function, function_prefix):
        workspace_creator(workspace_name)
        self.fit_browser.workspaceName = Mock(return_value=workspace_name)
        self.fit_browser.ignoreInvalidData.return_value = True
        self.fit_browser.currentHandler.return_value = self.create_mock_handler(function, function_prefix)

    def create_table_workspace(self, table_name):
        table = WorkspaceFactory.createTable()
        table.addColumn("double", X_COLUMN_LABEL, 1)
        table.addColumn("double", Y_COLUMN_LABEL, 2)
        for i in range(1, 10):
            table.addRow([0.1 * i, 5])
        AnalysisDataService.Instance().addOrReplace(table_name, table)
        self.fit_browser.getXColumnName.return_value = X_COLUMN_LABEL
        self.fit_browser.getYColumnName.return_value = Y_COLUMN_LABEL
        self.fit_browser.getErrColumnName.return_value = None
        self.fit_browser.startX.return_value = 0.15
        self.fit_browser.endX.return_value = 0.95

    def create_workspace2D(self, workspace_name):
        CreateSampleWorkspace(OutputWorkspace=workspace_name)
        self.fit_browser.workspaceIndex.return_value = 1
        self.fit_browser.startX.return_value = 0
        self.fit_browser.endX.return_value = 20000

    def create_mock_handler(self, function, function_prefix):
        mock_handler = MagicMock()
        mock_handler.ifun = MagicMock(return_value=function)
        mock_handler.functionPrefix = MagicMock(return_value=function_prefix)
        return mock_handler

    def create_mock_guess_lines(self):
        line_1, line_2, line_3 = MagicMock(spec=Line2D), MagicMock(spec=Line2D), MagicMock(spec=Line2D)
        mock_lines = [("f0." + FUNCTION_1.name(), line_1), ("f1." + FUNCTION_2.name(), line_2), ("f2." + FUNCTION_3.name(), line_3)]
        self.browser_plot_interaction.guess_lines = dict(mock_lines)
        return line_1, line_2, line_3

    def setUp(self):
        self.fit_browser = MagicMock(spec=FitPropertyBrowser)
        self.fit_browser.getFittingFunction = Mock(return_value=FULL_FUNCTION)
        # Mock figure
        self.canvas = MagicMock()
        self.figure = MagicMock()
        self.axes = MagicMock(spec=MantidAxes)
        self.axes.get_autoscale_on.return_value = False
        self.figure.get_axes.return_value = [self.axes]
        self.canvas.figure = self.figure
        self.browser_plot_interaction = FitPropertyBrowserPlotInteraction(self.fit_browser, self.canvas)

    def tearDown(self):
        AnalysisDataService.clear()

    def test_plot_guess_all_evaluates_correct_function(self):
        workspace_name = "test_workspace"
        self.setup_mock_fit_browser(self.create_workspace2D, workspace_name, FULL_FUNCTION, "")
        self.browser_plot_interaction.evaluate_function = Mock()

        self.browser_plot_interaction.plot_guess_all()

        self.browser_plot_interaction.evaluate_function.assert_called_once_with(workspace_name, FULL_FUNCTION, workspace_name + "_guess")

    def test_plot_guess_all_correctly_calls_plot(self):
        workspace_name = "test_workspace"
        self.setup_mock_fit_browser(self.create_workspace2D, workspace_name, FULL_FUNCTION, "")

        self.browser_plot_interaction.plot_guess_all()

        self.figure.get_axes.assert_called_once()
        self.axes.plot.assert_called_once_with(
            ANY, wkspIndex=1, label=workspace_name + "_guess", distribution=True, update_axes_labels=False, autoscale_on_update=False
        )

    def test_plot_current_guess_evaluates_correct_function(self):
        workspace_name = "test_workspace"
        prefix = "f1"
        self.setup_mock_fit_browser(self.create_workspace2D, workspace_name, FUNCTION_2, prefix)
        self.browser_plot_interaction.evaluate_function = Mock()

        self.browser_plot_interaction.plot_current_guess()

        self.browser_plot_interaction.evaluate_function.assert_called_once_with(
            workspace_name, FUNCTION_2, prefix + "." + FUNCTION_2.name()
        )

    def test_plot_current_guess_handles_invalid_function(self):
        workspace_name = "test_workspace"
        prefix = "f1"
        self.setup_mock_fit_browser(self.create_workspace2D, workspace_name, FUNCTION_2, prefix)
        self.browser_plot_interaction.evaluate_function = Mock()
        self.browser_plot_interaction.evaluate_function.side_effect = ValueError()

        self.browser_plot_interaction.plot_current_guess()
        self.axes.plot.assert_not_called()

    def test_plot_current_guess_correctly_calls_plot(self):
        workspace_name = "test_workspace"
        prefix = "f1"
        self.setup_mock_fit_browser(self.create_workspace2D, workspace_name, FUNCTION_2, prefix)

        self.browser_plot_interaction.plot_current_guess()

        self.figure.get_axes.assert_called_once()
        self.axes.plot.assert_called_once_with(
            ANY, wkspIndex=1, label=prefix + "." + FUNCTION_2.name(), distribution=True, update_axes_labels=False, autoscale_on_update=False
        )

    def test_plot_guess_doesnt_alter_autoscale(self):
        workspace_name = "test_workspace"
        prefix = "f1"
        self.setup_mock_fit_browser(self.create_workspace2D, workspace_name, FUNCTION_2, prefix)

        ax = self.figure.get_axes()[0]
        orig_autoscale = ax.get_autoscale_on()
        self.browser_plot_interaction.plot_current_guess()
        set_autoscale_call_count = ax.autoscale.call_count
        if set_autoscale_call_count > 0:
            ax.autoscale.assert_called_with(orig_autoscale, ANY)  # checks most recent call

    def test_plot_guess_all_plots_for_table_workspaces(self):
        table_name = "table_name"
        function = FUNCTION_2
        self.setup_mock_fit_browser(self.create_table_workspace, table_name, function, "")

        self.browser_plot_interaction.plot_guess_all()

        self.figure.get_axes.assert_called_once()
        self.axes.plot.assert_called_once_with(
            ANY, wkspIndex=1, label=table_name + "_guess", distribution=True, update_axes_labels=False, autoscale_on_update=False
        )

    def test_remove_function_correctly_updates_stored_prefixed_functions(self):
        workspace_name = "test_workspace"
        prefix = "f1"
        self.create_mock_guess_lines()
        self.setup_mock_fit_browser(self.create_workspace2D, workspace_name, FUNCTION_2, prefix)

        self.browser_plot_interaction.slot_for_function_removed()

        self.assertEqual(list(self.browser_plot_interaction.guess_lines.keys()), ["f0.FlatBackground", "f1.GausOsc"])

    def test_remove_function_correctly_removes_line(self):
        workspace_name = "test_workspace"
        prefix = "f1"
        line_1, line_2, line_3 = self.create_mock_guess_lines()

        self.setup_mock_fit_browser(self.create_workspace2D, workspace_name, FUNCTION_2, prefix)

        self.browser_plot_interaction.slot_for_function_removed()

        line_2.remove.assert_called_once()

    def test_remove_function_correctly_updates_legend(self):
        workspace_name = "test_workspace"
        prefix = "f1"
        line_1, line_2, line_3 = self.create_mock_guess_lines()
        self.setup_mock_fit_browser(self.create_workspace2D, workspace_name, FUNCTION_2, prefix)

        self.browser_plot_interaction.slot_for_function_removed()

        # Make legend will be called twice, once when removing the line and the second time to update the legend
        # based on the new prefixes
        self.assertEqual(self.axes.make_legend.call_count, 2)
        line_3.set_label.assert_called_once_with("f1.GausOsc")

    def test_remove_function_updates_guess_all(self):
        workspace_name = "test_workspace"
        prefix = "f1"
        old_line = MagicMock(spec=Line2D)
        self.browser_plot_interaction.guess_all_line = old_line
        self.setup_mock_fit_browser(self.create_workspace2D, workspace_name, FUNCTION_2, prefix)

        self.browser_plot_interaction.slot_for_function_removed()
        old_line.remove.assert_called_once()
        self.axes.plot.assert_called_once_with(
            ANY,
            wkspIndex=1,
            label=workspace_name + "_guess",
            distribution=True,
            update_axes_labels=False,
            autoscale_on_update=False,
            color=old_line.get_color(),
        )

    def test_changing_parameters_refreshes_guess_all(self):
        workspace_name = "test_workspace"
        prefix = "f1"
        old_line = MagicMock(spec=Line2D)
        self.browser_plot_interaction.guess_all_line = old_line
        self.setup_mock_fit_browser(self.create_workspace2D, workspace_name, FUNCTION_2, prefix)

        self.browser_plot_interaction.parameters_changed_slot("f1")

        old_line.remove.assert_called_once()
        self.axes.plot.assert_called_once_with(
            ANY,
            wkspIndex=1,
            label=workspace_name + "_guess",
            distribution=True,
            update_axes_labels=False,
            autoscale_on_update=False,
            color=old_line.get_color(),
        )

    def test_changing_parameters_refreshes_current_guess(self):
        workspace_name = "test_workspace"
        prefix = "f1"
        line_1, line_2, line_3 = self.create_mock_guess_lines()
        self.setup_mock_fit_browser(self.create_workspace2D, workspace_name, FUNCTION_2, prefix)

        self.browser_plot_interaction.parameters_changed_slot("f1")

        line_2.remove.assert_called_once()
        self.axes.plot.assert_called_once_with(
            ANY,
            wkspIndex=1,
            label=prefix + "." + FUNCTION_2.name(),
            distribution=True,
            update_axes_labels=False,
            autoscale_on_update=False,
            color=line_2.get_color(),
        )


if __name__ == "__main__":
    unittest.main()
