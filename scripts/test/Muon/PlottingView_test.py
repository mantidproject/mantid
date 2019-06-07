# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import os
import sys
import unittest

os.environ["QT_API"] = "pyqt"  # noqa E402

from matplotlib.figure import Figure

from mantid import WorkspaceFactory, plots
from mantid.py3compat import mock
from Muon.GUI.Common.test_helpers import mock_widget
from Muon.GUI.ElementalAnalysis.Plotting.subPlot_object import subPlot
from Muon.GUI.ElementalAnalysis.Plotting.plotting_view import PlotView
from Muon.GUI.ElementalAnalysis.Plotting.AxisChanger.axis_changer_presenter import AxisChangerPresenter


def get_subPlot(name):
    data_x, data_y = [1, 2, 3, 4], [4, 5, 6, 7]
    nspec = 2
    ws1 = WorkspaceFactory.create("Workspace2D", nspec, len(data_x), len(data_y))
    for i in range(ws1.getNumberHistograms()):
        ws1.setX(i, data_x)
        ws1.setY(i, data_y)
    label1 = "test"
    # create real lines
    fig = Figure()
    sub = fig.add_subplot(1, 1, 1)
    line1 = plots.plotfunctions.plot(sub, ws1, specNum=1)
    # add them both
    subplot = subPlot(name)
    subplot.addLine(label1, line1, ws1, 2)
    return subplot, ws1

@unittest.skipIf(lambda: sys.platform=='win32'(), "Test segfaults on Windows and code will be removed soon")
class PlottingViewHelperFunctionTests(unittest.TestCase):

    def setUp(self):
        self._qapp = mock_widget.mockQapp()

        self.view = PlotView()

        self.mock_func = mock.Mock(return_value=True)
        self.mock_args = [mock.Mock() for i in range(3)]
        self.mock_kwargs = {}
        self.mock_name = mock.Mock()

        self.plot_name = "test plot"
        self.plots_return_value = mock.Mock()
        self.plots_return_value.set_position = mock.Mock()
        self.plots_return_value.set_subplotspec = mock.Mock()
        self.plots_return_value.remove = mock.Mock()

        self.view.figure = mock.Mock()
        self.view.figure.tight_layout = mock.Mock()
        self.view.figure.add_subplot = mock.Mock(
            return_value=self.plots_return_value)
        self.view.canvas = mock.Mock()
        self.view.canvas.draw = mock.Mock()

        self.view.plots = {self.plot_name: self.plots_return_value}
        self.mock_plot = mock.Mock()
        self.mock_plot.get_xlim = mock.Mock()
        self.mock_plot.get_ylim = mock.Mock()
        self.mock_workspace = mock.Mock()

        self.mock_bounds = mock.Mock()

        self.mock_grid_pos = mock.Mock()
        self.view.current_grid = mock.Mock()
        self.view.current_grid.__getitem__ = mock.Mock(
            return_value=self.mock_grid_pos)
        self.mock_grid_pos.get_position = mock.Mock(return_value=True)

        self.view.gridspecs = mock.Mock()
        self.view.gridspecs.__getitem__ = mock.Mock(
            return_value=self.view.current_grid)

        self.view.plot_selector = mock.Mock()
        self.view.plot_selector.currentText = mock.Mock(
            return_value=self.plot_name)
        self.view.plot_selector.clear = mock.Mock()
        self.view.plot_selector.addItems = mock.Mock()

        self.view.x_axis_changer = mock.create_autospec(AxisChangerPresenter)
        self.view.y_axis_changer = mock.create_autospec(AxisChangerPresenter)

        self.view.errors = mock.Mock()
        self.view.errors.blockSignals = mock.Mock()
        self.view.errors.setChecked = mock.Mock()

    def test_redo_layout_with_plots_not_equal_to_zero(self):
        self.view.plots = [mock.Mock() for i in range(3)]
        # uses moveable_vline as this calls redo_layout, but has no other
        # functionality
        self.view.add_moveable_vline(*[mock.Mock() for i in range(4)])
        self.assertEqual(self.view.figure.tight_layout.call_count, 1)
        self.assertEqual(self.view.canvas.draw.call_count, 1)

    def test_redo_layout_with_plots_equal_to_zero(self):
        self.view.plots = []
        # uses moveable_vline as this calls redo_layout, but has no other
        # functionality
        self.view.add_moveable_vline(*[mock.Mock() for i in range(4)])
        self.assertEqual(self.view.canvas.draw.call_count, 1)

    def test_silent_checkbox_check(self):
        test_state = mock.Mock()
        self.view._silent_checkbox_check(test_state)
        self.view.errors.blockSignals.assert_has_calls(
            [mock.call(True), mock.call(False)])
        self.view.errors.setChecked.assert_called_once_with(test_state)

    def test_get_current_plot_name(self):
        self.assertEqual(self.view._get_current_plot_name(), self.plot_name)

    def _common_set_plot_bounds(self, result):
        self.view._silent_checkbox_check = mock.Mock()
        self.view._set_plot_bounds(self.plot_name, self.mock_plot)
        self.view.x_axis_changer.set_bounds.assert_called_once_with(
            self.mock_plot.get_xlim())
        self.view.y_axis_changer.set_bounds.assert_called_once_with(
            self.mock_plot.get_ylim())
        self.view._silent_checkbox_check.assert_called_once_with(result)

    def test_set_plot_bounds_in_errors_list(self):
        self.view.errors_list = [self.plot_name]
        self._common_set_plot_bounds(True)

    def test_set_plot_bounds_not_in_errors_list(self):
        self._common_set_plot_bounds(False)

    def test_set_bounds_when_new_plots(self):
        self.view._set_plot_bounds = mock.Mock()
        self.view.get_subplot = mock.Mock(return_value=self.mock_plot)
        self.view.plots = {self.plot_name: self.mock_plot}
        self.view._set_bounds(self.plot_name)
        self.view.get_subplot.assert_called_once_with(self.plot_name)
        self.view._set_plot_bounds.assert_called_once_with(
            self.plot_name, self.mock_plot)

    def common_set_bounds_else_statement(self, plot_name):
        self.view._set_bounds(plot_name)
        self.assertEqual(self.view.x_axis_changer.clear_bounds.call_count, 1)
        self.assertEqual(self.view.y_axis_changer.clear_bounds.call_count, 1)

    def test_set_bounds_when_not_new_plots(self):
        self.common_set_bounds_else_statement("")

    def test_set_bounds_when_all(self):
        self.common_set_bounds_else_statement("")

    def test_get_current_plots(self):
        self.assertEqual(
            self.view._get_current_plots(),
            [self.plots_return_value])

    def test_get_current_plots_raises_key_error(self):
        self.view.plots = {}
        with self.assertRaises(KeyError):
            self.view._get_current_plots()

    def test_update_x_axis_lower(self):
        self.view._update_x_axis = mock.Mock()
        self.view._update_x_axis_lower(self.mock_bounds)
        self.view._update_x_axis.assert_called_once_with(
            {"left": self.mock_bounds})

    def test_update_x_axis_upper(self):
        self.view._update_x_axis = mock.Mock()
        self.view._update_x_axis_upper(self.mock_bounds)
        self.view._update_x_axis.assert_called_once_with(
            {"right": self.mock_bounds})

    def test_update_x_axis(self):
        test_arg = {"left": self.mock_bounds}
        self.view._update_x_axis(test_arg)
        plot, = self.view._get_current_plots()
        plot.set_xlim.assert_called_once_with(left=self.mock_bounds)

    def test_update_y_axis_lower(self):
        self.view._update_y_axis = mock.Mock()
        self.view._update_y_axis_lower(self.mock_bounds)
        self.view._update_y_axis.assert_called_once_with(
            {"bottom": self.mock_bounds})

    def test_update_y_axis_upper(self):
        self.view._update_y_axis = mock.Mock()
        self.view._update_y_axis_upper(self.mock_bounds)
        self.view._update_y_axis.assert_called_once_with(
            {"top": self.mock_bounds})

    def test_update_y_axis(self):
        test_arg = {"top": self.mock_bounds}
        self.view._update_y_axis(test_arg)
        plot, = self.view._get_current_plots()
        plot.set_ylim.assert_called_once_with(top=self.mock_bounds)

    def test_modify_errors_list_state_is_true(self):
        self.view._modify_errors_list(self.plot_name, True)
        self.assertEqual(self.view.errors_list, set([self.plot_name]))

    def test_modify_errors_list_state_is_false(self):
        self.view.errors_list = set([self.plot_name])
        self.view._modify_errors_list(self.plot_name, False)
        self.assertEqual(len(self.view.errors_list), 0)

    def test_modify_errors_list_keyerror_thrown(self):
        test_set = set(["test"])
        self.view.errors_list = test_set
        self.view._modify_errors_list(self.plot_name, False)
        self.assertEqual(self.view.errors_list, test_set)

    def test_errors_changed_all(self):
        mock_state = True
        self.view._get_current_plot_name = mock.Mock(return_value="All")
        self.view._change_plot_errors = mock.Mock()
        self.view._errors_changed(mock_state)
        self.view._change_plot_errors.assert_called_once_with(
            self.plot_name, self.plots_return_value, mock_state)

    def test_errors_changed(self):
        mock_state = True
        self.view._get_current_plot_name = mock.Mock(
            return_value=self.plot_name)
        self.view._change_plot_errors = mock.Mock()
        self.view._errors_changed(mock_state)
        self.view._change_plot_errors.assert_called_once_with(
            self.plot_name, self.plots_return_value, mock_state)

    def test_change_plot_errors(self):
        args = [self.plot_name, self.plots_return_value, True]
        self.view._modify_errors_list = mock.Mock()
        self.view.plot = mock.Mock()
        # create subplot object
        subplot, ws = get_subPlot(self.plot_name)
        self.view.plot_storage = {self.plot_name: subplot}
        self.view.plot_storage[self.plot_name].delete = mock.Mock()

        self.view._change_plot_errors(*args)
        self.view._modify_errors_list.assert_called_once_with(*args[::2])
        self.assertEqual(
            self.view.plot_storage[
                self.plot_name].delete.call_count,
            1)
        self.view.plot.assert_called_once_with(
            self.plot_name, ws)

    def test_set_positions(self):
        self.view._set_positions([[0, 0]])
        self.plots_return_value.set_position.assert_called_once_with(
            self.mock_grid_pos.get_position())
        self.plots_return_value.set_subplotspec.assert_called_once_with(
            self.mock_grid_pos)

    def _set_update_plot_selector_and_set_positions_mocked(self):
        self.view._set_positions = mock.Mock()
        self.view._update_plot_selector = mock.Mock()

    @mock.patch("Muon.GUI.ElementalAnalysis.Plotting.plotting_utils.get_layout")
    def test_update_gridspec_if_new_plots(self, get_layout):
        self._set_update_plot_selector_and_set_positions_mocked()
        new_plots = [self.mock_plot]
        self.view._update_gridspec(new_plots)
        get_layout.assert_called_once_with(new_plots)
        self.view._set_positions.assert_called_once_with(get_layout())
        self.assertEqual(self.view._update_plot_selector.call_count, 1)

    @mock.patch("Muon.GUI.ElementalAnalysis.Plotting.plotting_utils.get_layout")
    def test_update_gridspec_if_new_plots_and_last(self, get_layout):
        self._set_update_plot_selector_and_set_positions_mocked()
        get_layout.return_value = [[0, 0]]
        self.view._update_gridspec([self.mock_plot], last=self.plot_name)
        self.view.figure.add_subplot.assert_called_once_with(
            self.mock_grid_pos, label=self.plot_name)
        self.plots_return_value.set_subplotspec.assert_called_once_with(
            self.mock_grid_pos)
        self.assertEqual(self.view._update_plot_selector.call_count, 1)

    def test_gridspec_if_not_new_plots(self):
        self.view._update_plot_selector = mock.Mock()
        self.view._update_gridspec([])
        self.assertEqual(self.view._update_plot_selector.call_count, 1)

    def test_update_plot_selector(self):
        self.view._update_plot_selector()
        self.assertEqual(self.view.plot_selector.clear.call_count, 1)
        self.view.plot_selector.addItems.assert_called_once_with(
            list(self.view.plots.keys()))


@unittest.skipIf(lambda: sys.platform=='win32'(), "Test segfaults on Windows and code will be removed soon")
class PlottingViewPlotFunctionsTests(unittest.TestCase):

    def setUp(self):
        self._qapp = mock_widget.mockQapp()

        self.view = PlotView()

        self.mock_func = mock.Mock(return_value=True)
        self.mock_args = [mock.Mock() for i in range(3)]
        self.mock_name = mock.Mock()

        self.plot_name = "test plot"
        self.plots_return_value = mock.Mock()

        self.view.figure = mock.Mock()
        self.view.figure.tight_layout = mock.Mock()
        self.view.figure.delaxes = mock.Mock()
        self.view.canvas = mock.Mock()
        self.view.canvas.draw = mock.Mock()

        self.mock_workspace = mock.Mock()

        self.view._update_gridspec = mock.Mock()

        self.view.errors = mock.Mock()
        self.view.errors.isChecked = mock.Mock(return_value=True)

        self.view._add_workspace_name = mock.Mock()
        self.view._set_bounds = mock.Mock()

        self.view.plots = {self.plot_name: self.plots_return_value}
        self.view.plot_additions = {self.plot_name: self.plots_return_value}

    def test_plot_errors_in_errors_list(self):
        self.view.errors_list = [self.plot_name]
        self.view.plot_workspace_errors = mock.Mock()
        self.view.plot(self.plot_name, self.mock_workspace)
        self.view.plot_workspace_errors.assert_called_once_with(
            self.plot_name, self.mock_workspace)
        self.view._set_bounds.assert_called_once_with(self.plot_name)

    def test_plot_errors_not_in_errors_list(self):
        self.view.plot_workspace = mock.Mock()
        self.view.plot(self.plot_name, self.mock_workspace)
        self.view.plot_workspace.assert_called_once_with(
            self.plot_name, self.mock_workspace)
        self.view._set_bounds.assert_called_once_with(self.plot_name)

    @mock.patch("mantid.plots.plotfunctions.errorbar")
    #@mock.patch("mantid.plots.plotfunctions.plot")
    def test_plot_workspace_errors(self, error_bar):  # , normal_plot):
        self.view._add_plotted_line = mock.Mock()
        error_bar.return_value = tuple([[] for i in range(3)])
        mock_line = mock.Mock()
        mock_line.get_label = mock.Mock(return_value="test")
        mock_line.remove = mock.Mock()
        with mock.patch("mantid.plots.plotfunctions.plot") as normal_plot:
            normal_plot.return_value = tuple([mock_line])
            self.view.plot_workspace_errors(
                self.plot_name,
                self.mock_workspace)
            self.assertEqual(error_bar.call_count, 1)
            self.assertEqual(self.view._add_plotted_line.call_count, 1)
            self.assertEqual(normal_plot.call_count, 1)

    @mock.patch("mantid.plots.plotfunctions.plot")
    def test_plot_workspace(self, plot):
        self.view._add_plotted_line = mock.Mock()
        plot.return_value = tuple([mock.Mock()])
        self.view.plot_workspace(self.plot_name, self.mock_workspace)
        self.assertEqual(plot.call_count, 1)

    def test_get_subplot_raises_key_error(self):
        self.view.plots = {}
        with self.assertRaises(KeyError):
            self.view.get_subplot(self.plot_name)

    def test_get_subplot(self):
        self.assertEquals(
            self.view.get_subplot(
                self.plot_name),
            self.plots_return_value)

    def test_get_subplots(self):
        self.assertEqual(self.view.get_subplots(), self.view.plots)

    def test_add_subplot(self):
        self.view.get_subplot = mock.Mock(return_value=True)
        return_value = self.view.add_subplot(self.mock_name)
        self.view._update_gridspec.assert_called_once_with(
            len(self.view.plots) + 1, last=self.mock_name)
        self.assertEqual(return_value, True)

    def test_remove_subplot(self):
        self.view.plot_storage = {self.plot_name: get_subPlot(self.plot_name)}
        self.view.subplotRemovedSignal = mock.Mock()
        self.view.remove_subplot(self.plot_name)
        self.view.subplotRemovedSignal.callled_once_with(self.plot_name)
        self.view.figure.delaxes.assert_called_once_with(
            self.plots_return_value)
        print(self.view.plots)
        for _dict in [self.view.plots, self.view.plot_storage]:
            self.assertEqual(_dict, {})
        self.view._update_gridspec.assert_called_once_with(
            len(self.view.plots))

    def test_remove_subplot_raise_key_error(self):
        self.view.plots = {}
        with self.assertRaises(KeyError):
            self.view.remove_subplot(self.plot_name)

    def test_add_moveable_vline(self):
        """
        To be added when moveable vlines are implemented.
        """
        pass

    def test_add_moveable_hline(self):
        """
        To be added when moveable hlines are implemented.
        """
        pass


if __name__ == "__main__":
    unittest.main()
