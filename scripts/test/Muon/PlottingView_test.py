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
        self.view.figure = mock.Mock()
        self.view.figure.tight_layout = mock.Mock()
        self.view.canvas = mock.Mock()
        self.view.canvas.draw = mock.Mock()

        self.new_plot = "test plot"
        self.plots_return_value = mock.Mock()
        self.view.plots = {self.new_plot: self.plots_return_value}
        self.view.workspaces = {self.new_plot: [self.plots_return_value]}
        self.mock_plot = mock.Mock()
        self.mock_plot.get_xlim = mock.Mock()
        self.mock_plot.get_ylim = mock.Mock()
        self.mock_workspace = mock.Mock()

        self.mock_bounds = mock.Mock()

        self.view.plot_selector = mock.Mock()
        self.view.plot_selector.currentText = mock.Mock(
            return_value=self.new_plot)
        self.view.plot_selector.clear = mock.Mock()
        self.view.plot_selector.addItems = mock.Mock()

        self.view.x_axis_changer = mock.create_autospec(AxisChangerPresenter)
        self.view.y_axis_changer = mock.create_autospec(AxisChangerPresenter)

    def test_redo_layout_with_plots_not_equal_to_zero(self):
        self.view.plots = [mock.Mock() for i in range(3)]
        self.view.call_plot_method(
            self.mock_name,
            self.mock_func,
            *self.mock_args)
        self.mock_func.assert_called_once_with(*self.mock_args)
        self.view.figure.tight_layout.assert_called_once()
        self.view.canvas.draw.assert_called_once()

    def test_redo_layout_with_plots_equal_to_zero(self):
        self.view.plots = []
        self.view.call_plot_method(
            self.mock_name,
            self.mock_func,
            *self.mock_args)
        self.mock_func.assert_called_once_with(*self.mock_args)
        self.view.canvas.draw.assert_called_once()

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
        output = tuple([self.mock_name, tuple(self.mock_args), {}])
        self.assertEquals(
            self.view.plot_additions[self.mock_name][1][1:], output)

    def test_set_bounds_when_new_plots(self):
        self.view.plots = {self.new_plot: self.mock_plot}
        self.view._set_bounds(self.new_plot)
        self.view.x_axis_changer.set_bounds.assert_called_once_with(
            self.mock_plot.get_xlim())
        self.view.y_axis_changer.set_bounds.assert_called_once_with(
            self.mock_plot.get_ylim())

    def test_set_bounds_when_not_new_plots(self):
        self.new_plot = ""
        self.view._set_bounds(self.new_plot)
        self.view.x_axis_changer.clear_bounds.assert_called_once()
        self.view.y_axis_changer.clear_bounds.assert_called_once()

    def test_get_current_plot(self):
        self.assertEquals(
            self.view._get_current_plot(),
            self.plots_return_value)

    def test_get_current_plot_raises_key_error(self):
        self.view.plots = {}
        with self.assertRaises(KeyError):
            self.view._get_current_plot()

    def test_update_x_axis(self):
        self.view._update_x_axis(self.mock_bounds)
        self.view._get_current_plot().set_xlim.assert_called_once_with(self.mock_bounds)

    def test_update_y_axis(self):
        self.view._update_y_axis(self.mock_bounds)
        self.view._get_current_plot().set_ylim.assert_called_once_with(self.mock_bounds)

    def test_errors_changed(self):
        pass

    def test_replay_additions(self):
        self.view.plot_additions = {
            self.new_plot: [
                (self.mock_func,
                 self.mock_name,
                 self.mock_args,
                 self.mock_kwargs)]}
        self.view._replay_additions(self.new_plot)
        args = [self.view, self.mock_name]
        args.extend(self.mock_args)
        self.mock_func.assert_called_once_with(*args)

    def test_set_positions(self):
        pass

    def test_update_gridspec_if_new_plots(self):
        pass

    def test_gridspec_if_not_new_plots(self):
        self.view._update_plot_selector = mock.Mock()
        self.view._update_gridspec([])
        self.view._update_plot_selector.assert_called_once()

    def test_update_plot_selector(self):
        self.view._update_plot_selector()
        self.view.plot_selector.clear.assert_called_once()
        self.view.plot_selector.addItems.assert_called_once_with(
            self.view.plots.keys())

    def test_add_workspace_name_if_not_in_workspaces(self):
        self.view._add_workspace_name(self.new_plot, self.mock_workspace)
        self.assertEquals(self.view.workspaces[self.new_plot], [
                          self.plots_return_value, self.mock_workspace])

    def test_add_workspace_name_if_in_workspaces(self):
        self.view.workspaces = {}
        self.view._add_workspace_name(self.new_plot, self.mock_workspace)
        self.assertEquals(self.view.workspaces[self.new_plot], [
                          self.mock_workspace])


class PlottingViewPlotFunctionsTests(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()

        self.view = PlotView()

        self.mock_func = mock.Mock(return_value=True)
        self.mock_args = [mock.Mock() for i in range(3)]
        self.mock_name = mock.Mock()

        self.new_plot = "test plot"
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

        self.view.plots = {self.new_plot: self.plots_return_value}
        self.view.workspaces = {self.new_plot: self.plots_return_value}
        self.view.plot_additions = {self.new_plot: self.plots_return_value}

    def test_plot_errors_checked(self):
        self.view.plot_workspace_errors = mock.Mock()
        self.view.plot(self.new_plot, self.mock_workspace)
        self.view.plot_workspace_errors.assert_called_once_with(
            self.new_plot, self.mock_workspace)
        self.view._set_bounds.assert_called_once_with(self.new_plot)

    def test_plot_errors_unchecked(self):
        self.view.errors.isChecked = mock.Mock(return_value=False)
        self.view.plot_workspace = mock.Mock()
        self.view.plot(self.new_plot, self.mock_workspace)
        self.view.plot_workspace.assert_called_once_with(
            self.new_plot, self.mock_workspace)
        self.view._set_bounds.assert_called_once_with(self.new_plot)

    @mock.patch("mantid.plots.plotfunctions.errorbar")
    def test_plot_workspace_errors(self, error_bar):
        self.view.plot_workspace_errors(self.new_plot, self.mock_workspace)
        error_bar.assert_called_once()

    @mock.patch("mantid.plots.plotfunctions.plot")
    def test_plot_workspace(self, plot):
        self.view.plot_workspace(self.new_plot, self.mock_workspace)
        plot.assert_called_once()

    def test_get_subplot_raises_key_error(self):
        self.view.plots = {}
        with self.assertRaises(KeyError):
            self.view.get_subplot(self.new_plot)

    def test_get_subplot(self):
        self.assertEquals(
            self.view.get_subplot(
                self.new_plot),
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
        self.view.remove_subplot(self.new_plot)
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
            self.view.remove_subplot(self.new_plot)

    def test_call_plot_method(self):
        self.view.call_plot_method(
            self.mock_name,
            self.mock_func,
            *self.mock_args)
        self.mock_func.assert_called_once_with(*self.mock_args)

    def test_add_vline(self):
        self.view.add_vline(self.new_plot, *self.mock_args)
        self.plots_return_value.axvline.assert_called_once_with(
            *self.mock_args)

    def test_add_vline_raises_key_error(self):
        self.view.plots = {}
        with self.assertRaises(KeyError):
            self.view.add_hline(self.mock_name, *self.mock_args)

    def test_add_hline(self):
        self.view.add_hline(self.new_plot, *self.mock_args)
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
