# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from typing import Dict
from unittest import mock

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.sliceviewer.toolbar import ToolItemText
from mantidqt.widgets.sliceviewer.view import SliceViewerDataView


@start_qapplication
class SliceviewerDataViewTest(unittest.TestCase):
    def setUp(self) -> None:
        self.patcher = mock.patch.multiple("mantidqt.widgets.sliceviewer.view",
                                           DimensionWidget=mock.DEFAULT,
                                           QGridLayout=mock.DEFAULT,
                                           QHBoxLayout=mock.DEFAULT,
                                           QVBoxLayout=mock.DEFAULT,
                                           QWidget=mock.DEFAULT,
                                           QLabel=mock.DEFAULT,
                                           QStatusBar=mock.DEFAULT,
                                           CurveLinearSubPlot=mock.DEFAULT,
                                           ColorbarWidget=mock.DEFAULT,
                                           SliceViewerNavigationToolbar=mock.DEFAULT)
        self.patched_objs: Dict[mock.Mock] = self.patcher.start()
        self.presenter = mock.Mock()
        self.view = SliceViewerDataView(presenter=self.presenter, dims_info=mock.MagicMock(), can_normalise=mock.Mock())

    def tearDown(self) -> None:
        self.patcher.stop()

    def test_mpl_toolbar_signals_connected(self):
        self.view.mpl_toolbar.gridClicked.connect.assert_called_once_with(self.view.toggle_grid)
        self.view.mpl_toolbar.linePlotsClicked.connect.assert_called_once_with(self.view.on_line_plots_toggle)
        self.view.mpl_toolbar.regionSelectionClicked.connect.assert_called_once_with(self.view.on_region_selection_toggle)
        self.view.mpl_toolbar.homeClicked.connect.assert_called_once_with(self.view.on_home_clicked)
        self.view.mpl_toolbar.nonOrthogonalClicked.connect.assert_called_once_with(self.view.on_non_orthogonal_axes_toggle)
        self.view.mpl_toolbar.zoomPanClicked.connect.assert_called_once_with(self.view.presenter.deactivate_peak_adding)
        self.view.mpl_toolbar.zoomPanFinished.connect.assert_called_once_with(self.view.on_data_limits_changed)

    def test_color_bar_signals_connected(self):
        self.view.colorbar.colorbarChanged.connect.assert_called_once_with(self.view.update_data_clim)
        self.view.colorbar.scaleNormChanged.connect.assert_called_once_with(self.view.scale_norm_changed)

    def test_update_plot_data_non_orthogonal(self):
        self.view.nonortho_transform = True
        self.view.image = mock.Mock()
        data_mock = mock.Mock()

        self.view.update_plot_data(data_mock)

        self.view.image.set_array.assert_called_once_with(data_mock.T.ravel())
        self.view.colorbar.update_clim.assert_called_once_with()

    def test_update_plot_data_orthognal(self):
        self.view.nonortho_transform = False
        self.view.image = mock.Mock()
        data_mock = mock.Mock()

        self.view.update_plot_data(data_mock)

        self.view.image.set_data.assert_called_once_with(data_mock.T)
        self.view.colorbar.update_clim.assert_called_once_with()

    def test_update_data_clim(self):
        self.view.image = mock.Mock()
        self.view.canvas = mock.Mock()

        self.view.update_data_clim()

        self.view.image.set_clim.assert_called_once_with(self.view.colorbar.colorbar.mappable.get_clim())
        self.view.canvas.draw_idle.assert_called_once()

    def test_update_data_clim_updates_line_plot_limits_if_line_plots_active(self):
        self.view.image = mock.Mock()
        self.view._line_plots = mock.Mock()
        self.view.line_plots_active = True

        self.view.update_data_clim()

        self.view._line_plots.plotter.update_line_plot_limits.assert_called_once()

    def test_create_axes_non_orthogonal(self):
        self.view.fig = mock.Mock()
        self.view.canvas = mock.Mock()
        self.view.clear_figure = mock.Mock()

        self.view.create_axes_nonorthogonal(mock.NonCallableMock())

        self.view.clear_figure.assert_called_once()
        self.assertTrue(self.view.nonorthogonal_mode)
        self.patched_objs["CurveLinearSubPlot"].assert_called_once_with(self.view.fig, 1, 1, 1, grid_helper=mock.ANY)
        self.view.canvas.enable_zoom_on_scroll.assert_called_once_with(mock.ANY, redraw=False, toolbar=mock.ANY, callback=mock.ANY)
        self.assertTrue(self.view.grid_on)
        self.view.fig.add_subplot.assert_called_once()
        self.view.canvas.draw_idle.assert_called_once()

    def test_create_axes_orthogonal(self):
        self.view.fig = mock.Mock()
        self.view.clear_figure = mock.Mock()
        self.view._grid_on = mock.NonCallableMock()

        self.view.create_axes_orthogonal()

        self.view.clear_figure.assert_called_once()
        self.assertFalse(self.view.nonortho_transform)
        self.view.fig.add_subplot.assert_called_once_with(111, projection='mantid')
        self.view.ax.grid.assert_called_once_with(self.view._grid_on)

    def test_create_axes_orthogonal_redraws_on_zoom(self):
        for redraw in [True, False]:
            self.view.canvas = mock.Mock()
            self.view.create_axes_orthogonal(redraw_on_zoom=redraw)
            self.view.canvas.enable_zoom_on_scroll.assert_called_once_with(mock.ANY, redraw=redraw, toolbar=mock.ANY,
                                                                           callback=mock.ANY)

    def test_set_grid_on(self):
        self.view.set_grid_on()
        self.patched_objs["SliceViewerNavigationToolbar"].return_value.\
            set_action_checked.assert_called_once_with(ToolItemText.GRID, state=True)

    def test_clear_figure(self):
        self.view.ax = mock.NonCallableMock()
        self.view.image = mock.NonCallableMock()
        self.view.canvas = mock.Mock()
        self.view.fig = mock.Mock()

        self.view.clear_figure()

        self.assertIsNone(self.view.image)
        self.assertIsNone(self.view.ax)
        self.view.canvas.disable_zoom_on_scroll.assert_called_once()
        self.view.fig.clf.assert_called_once()

    def test_clear_figure_line_plots_false(self):
        for line_plots_active in [True, False]:
            self.view._line_plots = mock.Mock()
            self.view.line_plots_active = line_plots_active
            self.view.clear_figure()

            if line_plots_active:
                self.view._line_plots.plotter.close.assert_called_once()
            else:
                self.view._line_plots.plotter.close.assert_not_called()
            self.assertFalse(self.view.line_plots_active)

    def test_remove_line_plots(self):
        for line_plots_active in [True, False]:
            line_plots_mock = mock.Mock()
            self.view._line_plots = line_plots_mock
            self.view.line_plots_active = line_plots_active
            self.view.remove_line_plots()

            if line_plots_active:
                line_plots_mock.plotter.close.assert_called_once()
                self.view.status_bar_label.clear.assert_called_once()
                self.assertIsNone(self.view._line_plots)
                self.assertFalse(self.view.line_plots_active)
            else:
                self.assertIsNotNone(self.view._line_plots)
                line_plots_mock.plotter.close.assert_not_called()

    def test_plot_matrix(self):
        self.view.image = None
        self.view.clear_image = mock.Mock()
        self.view.ax = mock.Mock()
        self.view.colorbar = mock.Mock()

        ws = mock.NonCallableMock()
        self.view.plot_matrix(ws)
        self.view.clear_image.assert_called_once()
        self.view.ax.imshow.assert_called_once_with(ws, origin=mock.ANY, aspect=mock.ANY,
                                                    interpolation=mock.ANY, transpose=self.view.dimensions.transpose,
                                                    norm=self.view.colorbar.get_norm(), extent=mock.ANY)

    def test_plot_matrix_with_image(self):
        original_image = mock.Mock()
        self.view.image = original_image
        self.view.image.transpose = self.view.dimensions.transpose
        self.view.clear_image = mock.Mock()
        self.view.ax = mock.Mock()
        self.view.colorbar = mock.Mock()

        ws = mock.NonCallableMock()
        self.view.plot_matrix(ws)
        self.view.clear_image.assert_called_once()
        self.view.ax.imshow.assert_called_once_with(ws, origin=mock.ANY, aspect=mock.ANY,
                                                    interpolation=mock.ANY, transpose=self.view.dimensions.transpose,
                                                    norm=self.view.colorbar.get_norm(),
                                                    extent=original_image.get_extent())

    def test_draw_plot_clears_title(self):
        self.view.ax = mock.Mock()

        self.view.draw_plot()

        self.view.ax.set_title.assert_called_once_with('')

    def test_draw_plot_draws_canvas(self):
        self.view.ax = mock.Mock()
        self.view.canvas = mock.Mock()

        self.view.draw_plot()

        self.view.canvas.draw.assert_called_once()

    def test_draw_plot_updates_toolbar(self):
        self.view.ax = mock.Mock()

        self.view.draw_plot()

        self.view.mpl_toolbar.update.assert_called_once()

    def test_draw_plot_updates_colorbar(self):
        self.view.ax = mock.Mock()
        self.view.image = mock.NonCallableMock()

        self.view.draw_plot()

        self.view.colorbar.set_mappable.assert_called_once_with(self.view.image)
        self.view.colorbar.update_clim.assert_called_once()

    def test_draw_plot_does_not_update_colorbar_if_image_is_none(self):
        self.view.ax = mock.Mock()
        self.view.image = None

        self.view.draw_plot()

        self.view.colorbar.set_mappable.assert_not_called()
        self.view.colorbar.update_clim.assert_not_called()

    def test_draw_plot_updates_line_plots_if_active(self):
        self.view.ax = mock.Mock()
        self.view.image = mock.NonCallableMock()
        self.view._line_plots = mock.Mock()
        self.view.line_plots_active = True

        self.view.draw_plot()

        self.view._line_plots.plotter.delete_line_plot_lines.assert_called_once()
        self.view._line_plots.plotter.update_line_plot_labels.assert_called_once()

    def test_draw_plot_does_not_update_line_plots_if_inactive(self):
        self.view.ax = mock.Mock()
        self.view.image = mock.NonCallableMock()
        self.view._line_plots = mock.Mock()
        self.view.line_plots_active = False

        self.view.draw_plot()

        self.view._line_plots.plotter.delete_line_plot_lines.assert_not_called()
        self.view._line_plots.plotter.update_line_plot_labels.assert_not_called()


if __name__ == '__main__':
    unittest.main()
