import unittest

from Muon.GUI.ElementalAnalysis.Plotting.plotting_view import PlotView
from Muon.GUI.ElementalAnalysis.Plotting.AxisChanger.axis_changer_presenter import AxisChangerPresenter

from Muon.GUI.Common import mock_widget

try:
    from unittest import mock
except ImportError:
    import mock


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
        self.plots_return_value.clear = mock.Mock()

        self.view.figure = mock.Mock()
        self.view.figure.tight_layout = mock.Mock()
        self.view.figure.add_subplot = mock.Mock(
            return_value=self.plots_return_value)
        self.view.canvas = mock.Mock()
        self.view.canvas.draw = mock.Mock()

        self.view.plots = {self.plot_name: self.plots_return_value}
        self.view.workspaces = {self.plot_name: [self.plots_return_value]}
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
        self.view.call_plot_method(
            self.mock_name,
            self.mock_func,
            *self.mock_args)
        self.mock_func.assert_called_once_with(*self.mock_args)
        self.assertEquals(self.view.figure.tight_layout.call_count, 1)
        self.assertEquals(self.view.canvas.draw.call_count, 1)

    def test_redo_layout_with_plots_equal_to_zero(self):
        self.view.plots = []
        self.view.call_plot_method(
            self.mock_name,
            self.mock_func,
            *self.mock_args)
        self.mock_func.assert_called_once_with(*self.mock_args)
        self.assertEquals(self.view.canvas.draw.call_count, 1)

    def test_save_addition_called_once(self):
        self.view.call_plot_method(
            self.mock_name,
            self.mock_func,
            *self.mock_args)
        self.mock_args.insert(0, self.mock_func)
        output = tuple([self.mock_name, tuple(self.mock_args), {}])
        self.assertEquals(
            self.view.plot_additions[self.mock_name][0][1:], output)

    def test_save_addition_called_twice(self):
        for i in range(2):
            self.view.call_plot_method(
                self.mock_name, self.mock_func, *self.mock_args)
        self.mock_args.insert(0, self.mock_func)
        output = (self.mock_name, tuple(self.mock_args), {})
        self.assertEquals(
            self.view.plot_additions[self.mock_name][1][1:], output)

    def test_silent_checkbox_check(self):
        test_state = mock.Mock()
        self.view._silent_checkbox_check(test_state)
        self.view.errors.blockSignals.assert_has_calls(
            [mock.call(True), mock.call(False)])
        self.view.errors.setChecked.assert_called_once_with(test_state)

    def test_get_current_plot_name(self):
        self.assertEquals(self.view._get_current_plot_name(), self.plot_name)

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
        self.assertEquals(self.view.x_axis_changer.clear_bounds.call_count, 1)
        self.assertEquals(self.view.y_axis_changer.clear_bounds.call_count, 1)

    def test_set_bounds_when_not_new_plots(self):
        self.common_set_bounds_else_statement("")

    def test_set_bounds_when_all(self):
        self.common_set_bounds_else_statement("")

    def test_get_current_plots(self):
        self.assertEquals(
            self.view._get_current_plots(),
            [self.plots_return_value])

    def test_get_current_plots_raises_key_error(self):
        self.view.plots = {}
        with self.assertRaises(KeyError):
            self.view._get_current_plots()

    def test_update_x_axis(self):
        self.view._update_x_axis(self.mock_bounds)
        plot, = self.view._get_current_plots()
        plot.set_xlim.assert_called_once_with(self.mock_bounds)

    def test_update_y_axis(self):
        self.view._update_y_axis(self.mock_bounds)
        plot, = self.view._get_current_plots()
        plot.set_ylim.assert_called_once_with(self.mock_bounds)

    def test_modify_errors_list_state_is_true(self):
        self.view._modify_errors_list(self.plot_name, True)
        self.assertEquals(self.view.errors_list, set([self.plot_name]))

    def test_modify_errors_list_state_is_false(self):
        self.view.errors_list = set([self.plot_name])
        self.view._modify_errors_list(self.plot_name, False)
        self.assertEquals(len(self.view.errors_list), 0)

    def test_modify_errors_list_keyerror_thrown(self):
        test_set = set(["test"])
        self.view.errors_list = test_set
        self.view._modify_errors_list(self.plot_name, False)
        self.assertEquals(self.view.errors_list, test_set)

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
        self.view._replay_additions = mock.Mock()
        self.view._change_plot_errors(*args)
        self.view._modify_errors_list.assert_called_once_with(*args[::2])
        self.assertEquals(self.plots_return_value.clear.call_count, 1)
        self.view.plot.assert_called_once_with(
            self.plot_name, self.plots_return_value)
        self.view._replay_additions.assert_called_once_with(self.plot_name)

    def test_replay_additions(self):
        self.view.plot_additions = {
            self.plot_name: [
                (self.mock_func,
                 self.mock_name,
                 self.mock_args,
                 self.mock_kwargs)]}
        self.view._replay_additions(self.plot_name)
        args = [self.view, self.mock_name]
        args.extend(self.mock_args)
        self.mock_func.assert_called_once_with(*args)

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
        self.assertEquals(self.view._update_plot_selector.call_count, 1)

    @mock.patch("Muon.GUI.ElementalAnalysis.Plotting.plotting_utils.get_layout")
    def test_update_gridspec_if_new_plots_and_last(self, get_layout):
        self._set_update_plot_selector_and_set_positions_mocked()
        get_layout.return_value = [[0, 0]]
        self.view._update_gridspec([self.mock_plot], last=self.plot_name)
        self.view.figure.add_subplot.assert_called_once_with(
            self.mock_grid_pos, label=self.plot_name)
        self.plots_return_value.set_subplotspec.assert_called_once_with(
            self.mock_grid_pos)
        self.assertEquals(self.view._update_plot_selector.call_count, 1)

    def test_gridspec_if_not_new_plots(self):
        self.view._update_plot_selector = mock.Mock()
        self.view._update_gridspec([])
        self.assertEquals(self.view._update_plot_selector.call_count, 1)

    def test_update_plot_selector(self):
        self.view._update_plot_selector()
        self.assertEquals(self.view.plot_selector.clear.call_count, 1)
        self.view.plot_selector.addItems.assert_called_once_with(
            self.view.plots.keys())

    def test_add_workspace_name_if_not_in_workspaces(self):
        self.view._add_workspace_name(self.plot_name, self.mock_workspace)
        self.assertEquals(self.view.workspaces[self.plot_name], [
                          self.plots_return_value, self.mock_workspace])

    def test_add_workspace_name_if_in_workspaces(self):
        self.view.workspaces = {}
        self.view._add_workspace_name(self.plot_name, self.mock_workspace)
        self.assertEquals(self.view.workspaces[self.plot_name], [
                          self.mock_workspace])


class PlottingViewPlotFunctionsTests(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()

        self.view = PlotView()

        self.mock_func = mock.Mock(return_value=True)
        self.mock_args = [mock.Mock() for i in range(3)]
        self.mock_name = mock.Mock()

        self.plot_name = "test plot"
        self.plots_return_value = mock.Mock()
        self.plots_return_value.axvline = mock.Mock()
        self.plots_return_value.ahvline = mock.Mock()

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
        self.view.workspaces = {self.plot_name: self.plots_return_value}
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
    def test_plot_workspace_errors(self, error_bar):
        self.view.plot_workspace_errors(self.plot_name, self.mock_workspace)
        self.assertEquals(error_bar.call_count, 1)

    @mock.patch("mantid.plots.plotfunctions.plot")
    def test_plot_workspace(self, plot):
        self.view.plot_workspace(self.plot_name, self.mock_workspace)
        self.assertEquals(plot.call_count, 1)

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
        self.assertEquals(self.view.get_subplots(), self.view.plots)

    def test_add_subplot(self):
        self.view.get_subplot = mock.Mock(return_value=True)
        return_value = self.view.add_subplot(self.mock_name)
        self.view._update_gridspec.assert_called_once_with(
            len(self.view.plots) + 1, last=self.mock_name)
        self.assertEquals(return_value, True)

    def test_remove_subplot(self):
        self.view.remove_subplot(self.plot_name)
        self.view.figure.delaxes.assert_called_once_with(
            self.plots_return_value)
        for _dict in [self.view.plots, self.view.workspaces,
                      self.view.plot_additions]:
            self.assertEquals(_dict, {})
        self.view._update_gridspec.assert_called_once_with(
            len(self.view.plots))

    def test_remove_subplot_raise_key_error(self):
        self.view.plots = {}
        with self.assertRaises(KeyError):
            self.view.remove_subplot(self.plot_name)

    def test_call_plot_method(self):
        self.view.call_plot_method(
            self.mock_name,
            self.mock_func,
            *self.mock_args)
        self.mock_func.assert_called_once_with(*self.mock_args)

    def test_add_vline(self):
        self.view.add_vline(self.plot_name, *self.mock_args)
        self.plots_return_value.axvline.assert_called_once_with(
            *self.mock_args)

    def test_add_vline_raises_key_error(self):
        self.view.plots = {}
        with self.assertRaises(KeyError):
            self.view.add_hline(self.mock_name, *self.mock_args)

    def test_add_hline(self):
        self.view.add_hline(self.plot_name, *self.mock_args)
        self.plots_return_value.axhline.assert_called_once_with(
            *self.mock_args)

    def test_add_hline_raises_key_error(self):
        self.view.plots = {}
        with self.assertRaises(KeyError):
            self.view.add_vline(self.mock_name, *self.mock_args)

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
