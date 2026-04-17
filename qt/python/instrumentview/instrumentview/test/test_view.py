# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from unittest.mock import MagicMock

import numpy as np
from qtpy.QtCore import Qt
from mantidqt.utils.qt.testing import start_qapplication
from mantid.simpleapi import CreateSampleWorkspace
from instrumentview.FullInstrumentViewWindow import FullInstrumentViewWindow
from instrumentview.ShapeWidgets import (
    AnnulusSelectionShape,
    CircleSelectionShape,
    HollowRectangleSelectionShape,
    RectangleSelectionShape,
    EllipseSelectionShape,
)


@start_qapplication
class TestFullInstrumentViewWindow(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls._workspace = CreateSampleWorkspace(StoreInADS=False)

    @mock.patch("instrumentview.FullInstrumentViewWindow.FigureCanvas")
    @mock.patch("qtpy.QtWidgets.QHBoxLayout.addWidget")
    @mock.patch("qtpy.QtWidgets.QVBoxLayout.addWidget")
    @mock.patch("qtpy.QtWidgets.QSplitter.addWidget")
    @mock.patch("instrumentview.FullInstrumentViewWindow.BackgroundPlotter")
    def setUp(self, mock_plotter, mock_splitter_add_widget, mock_v_add_widget, mock_h_add_widget, mock_figure_canvas) -> None:
        with mock.patch("mantidqt.utils.qt.qappthreadcall.force_method_calls_to_qapp_thread"):
            self._view = FullInstrumentViewWindow()
        self._mock_plotter = mock_plotter
        self._mock_splitter_add_widget = mock_splitter_add_widget
        self._mock_v_add_widget = mock_v_add_widget
        self._mock_h_add_widget = mock_h_add_widget
        self._mock_figure_canvas = mock_figure_canvas
        self._view._presenter = MagicMock()

    def test_plotters_created(self):
        self._mock_plotter.assert_called_once()

    def test_select_bank_tube_button_is_checkable(self):
        self.assertEqual(self._view._select_bank_tube.text(), "Select Bank/Tube")
        self.assertTrue(self._view._select_bank_tube.isCheckable())

    def test_is_select_bank_tube_checked(self):
        self.assertFalse(self._view.is_select_bank_tube_checked())
        self._view._select_bank_tube.setChecked(True)
        self.assertTrue(self._view.is_select_bank_tube_checked())

    def test_figure_canvas_created(self):
        self._mock_figure_canvas.assert_called_once()

    def test_update_scalar_range(self):
        self._view.set_plotter_scalar_bar_range((0, 100), "label")
        self._view.main_plotter.update_scalar_bar_range.assert_has_calls([mock.call((0, 100), "label")])

    @mock.patch("qtpy.QtWidgets.QMainWindow.closeEvent")
    def test_close_event(self, mock_close_event):
        self._view.closeEvent(MagicMock())
        self.assertEqual(1, mock_close_event.call_count)
        self._view.main_plotter.close.assert_called_once()

    @mock.patch("qtpy.QtWidgets.QMainWindow.closeEvent")
    def test_close_no_presenter(self, mock_close_event):
        self._view._presenter = None
        self._view.closeEvent(MagicMock())
        self._view.main_plotter.close.assert_called_once()

    def test_add_simple_shape(self):
        self._view.main_plotter.reset_mock()
        mock_mesh = MagicMock()
        mock_colour = MagicMock()
        self._view.add_simple_shape(mock_mesh, mock_colour, False)
        self._view.main_plotter.add_mesh.assert_called_once_with(mock_mesh, color=mock_colour, pickable=False)

    def test_add_rgba_mesh(self):
        self._view.main_plotter.reset_mock()
        mock_mesh = MagicMock()
        mock_scalars = MagicMock()
        self._view.add_rgba_mesh(mock_mesh, mock_scalars)
        self._view.main_plotter.add_mesh.assert_called_once_with(
            mock_mesh, scalars=mock_scalars, rgba=True, pickable=False, render_points_as_spheres=True, point_size=10
        )

    @mock.patch("instrumentview.FullInstrumentViewWindow.QListWidgetItem")
    def test_refresh_peaks_ws_list(self, mock_qlist_widget_item):
        mock_list = MagicMock()
        mock_item = MagicMock()
        mock_item.text.return_value = "existing_ws"
        mock_list.item.return_value = mock_item
        mock_list.count.return_value = 1
        self._view._peak_ws_list = mock_list
        self._view._presenter.peaks_workspaces_in_ads.return_value = ["existing_ws", "new_ws"]
        self._view.refresh_peaks_ws_list()
        # Only the new workspace should trigger QListWidgetItem creation
        mock_qlist_widget_item.assert_called_once()

    def test_clear_overlay_meshes(self):
        mock_meshes = (MagicMock(), MagicMock())
        self._view._overlay_meshes = [mock_meshes]
        self._view.clear_overlay_meshes()
        self._view.main_plotter.remove_actor.assert_has_calls([mock.call(mock_meshes[0]), mock.call(mock_meshes[1])])
        self.assertEqual(0, len(self._view._overlay_meshes))

    def test_clear_lineplot_overlays(self):
        mock_line = MagicMock()
        mock_text = MagicMock()
        self._view._detector_spectrum_axes = MagicMock()
        self._view._detector_spectrum_axes.texts = [mock_text]
        self.assertEqual(0, len(self._view._lineplot_overlays))
        self._view._lineplot_overlays.append(mock_line)
        self._view.clear_lineplot_overlays()
        mock_line.remove.assert_called_once()
        self.assertEqual(0, len(self._view._lineplot_overlays))
        mock_text.remove.assert_called_once()

    def test_plot_overlay_meshes(self):
        positions = [np.array([[0, 0, 0]])]
        labels = [["label"]]
        selected_workspaces = ["ws1"]
        mock_item = MagicMock()
        mock_item.foreground().color().name.return_value = "#ff7f0e"
        self._view._peak_ws_list = MagicMock()
        self._view._peak_ws_list.findItems.return_value = [mock_item]
        self._view.plot_overlay_meshes(positions, labels, selected_workspaces)
        self._view.main_plotter.add_points.assert_called_once()
        self._view.main_plotter.add_point_labels.assert_called_once()
        self.assertEqual(1, len(self._view._overlay_meshes))

    def test_plot_lineplot_peak_overlays(self):
        x_values = [[1.0, 2.0]]
        labels = [["a", "b"]]
        selected_workspaces = ["ws1"]
        mock_item = MagicMock()
        mock_item.foreground().color().name.return_value = "#ff7f0e"
        self._view._peak_ws_list = MagicMock()
        self._view._peak_ws_list.findItems.return_value = [mock_item]
        self._view._detector_spectrum_axes = MagicMock()
        self._view.plot_lineplot_peak_overlays(x_values, labels, selected_workspaces)
        self.assertEqual(2, self._view._detector_spectrum_axes.text.call_count)
        self.assertEqual(2, len(self._view._lineplot_overlays))

    def test_redraw_lineplot(self):
        self._view.redraw_lineplot()
        self._view._detector_figure_canvas.draw.assert_called_once()

    def test_add_rectangular_widget(self) -> None:
        self._view.add_rectangular_widget()
        self.assertIsInstance(self._view._current_widget, RectangleSelectionShape)
        self.assertIsNotNone(self._view._shape_overlay_manager)
        self.assertIs(self._view._shape_overlay_manager.current_shape, self._view._current_widget)

    def test_add_circle_widget(self) -> None:
        self._view.add_circle_widget()
        self.assertIsInstance(self._view._current_widget, CircleSelectionShape)
        self.assertIsNotNone(self._view._shape_overlay_manager)
        self.assertIs(self._view._shape_overlay_manager.current_shape, self._view._current_widget)

    def test_add_ellipse_widget(self) -> None:
        self._view.add_ellipse_widget()
        self.assertIsInstance(self._view._current_widget, EllipseSelectionShape)
        self.assertIsNotNone(self._view._shape_overlay_manager)
        self.assertIs(self._view._shape_overlay_manager.current_shape, self._view._current_widget)

    def test_add_annulus_widget(self) -> None:
        self._view.add_annulus_widget()
        self.assertIsInstance(self._view._current_widget, AnnulusSelectionShape)
        self.assertIsNotNone(self._view._shape_overlay_manager)
        self.assertIs(self._view._shape_overlay_manager.current_shape, self._view._current_widget)

    def test_add_hollow_rectangle_widget(self) -> None:
        self._view.add_hollow_rectangle_widget()
        self.assertIsInstance(self._view._current_widget, HollowRectangleSelectionShape)
        self.assertIsNotNone(self._view._shape_overlay_manager)
        self.assertIs(self._view._shape_overlay_manager.current_shape, self._view._current_widget)

    def test_delete_current_widget(self) -> None:
        self._view.add_circle_widget()
        self.assertIsNotNone(self._view._current_widget)
        self._view.delete_current_widget()
        self.assertIsNone(self._view._current_widget)
        self.assertIsNone(self._view._shape_overlay_manager)

    @mock.patch("instrumentview.FullInstrumentViewWindow.ConfigService")
    def test_store_draw_shapes_option_stores_yes_when_checked(self, mock_config):
        self._view._show_shapes_check_box.setChecked(True)
        self._view.store_draw_shapes_option()
        mock_config.Instance.return_value.__setitem__.assert_called_once_with(self._view._DRAW_SHAPES_SETTING_STRING, "Yes")

    def test_on_axes_click_left_calls_presenter_with_left(self):
        event = MagicMock()
        event.inaxes = self._view._detector_spectrum_axes
        event.xdata = 5.0
        event.button = 1
        self._view._on_axes_click(event)
        self._view._presenter.on_peak_selected_in_lineplot.assert_called_once_with(5.0, "left")

    @mock.patch("instrumentview.FullInstrumentViewWindow.ConfigService")
    def test_store_draw_shapes_option_stores_no_when_unchecked(self, mock_config):
        self._view._show_shapes_check_box.setChecked(False)
        self._view.store_draw_shapes_option()
        mock_config.Instance.return_value.__setitem__.assert_called_once_with(self._view._DRAW_SHAPES_SETTING_STRING, "No")

    @mock.patch("instrumentview.FullInstrumentViewWindow.FigureCanvas")
    @mock.patch("qtpy.QtWidgets.QHBoxLayout.addWidget")
    @mock.patch("qtpy.QtWidgets.QVBoxLayout.addWidget")
    @mock.patch("qtpy.QtWidgets.QSplitter.addWidget")
    @mock.patch("instrumentview.FullInstrumentViewWindow.BackgroundPlotter")
    @mock.patch("instrumentview.FullInstrumentViewWindow.ConfigService")
    def test_draw_shapes_checkbox_initialised_checked_when_config_is_yes(
        self, mock_config, mock_plotter, mock_splitter, mock_v_layout, mock_h_layout, mock_canvas
    ):
        mock_config.Instance.return_value.__getitem__.return_value = "Yes"
        with mock.patch("mantidqt.utils.qt.qappthreadcall.force_method_calls_to_qapp_thread"):
            view = FullInstrumentViewWindow()
        self.assertTrue(view.is_show_shapes_checkbox_checked())

    @mock.patch("instrumentview.FullInstrumentViewWindow.FigureCanvas")
    @mock.patch("qtpy.QtWidgets.QHBoxLayout.addWidget")
    @mock.patch("qtpy.QtWidgets.QVBoxLayout.addWidget")
    @mock.patch("qtpy.QtWidgets.QSplitter.addWidget")
    @mock.patch("instrumentview.FullInstrumentViewWindow.BackgroundPlotter")
    @mock.patch("instrumentview.FullInstrumentViewWindow.ConfigService")
    def test_draw_shapes_checkbox_initialised_unchecked_when_config_is_no(
        self, mock_config, mock_plotter, mock_splitter, mock_v_layout, mock_h_layout, mock_canvas
    ):
        mock_config.Instance.return_value.__getitem__.return_value = "No"
        with mock.patch("mantidqt.utils.qt.qappthreadcall.force_method_calls_to_qapp_thread"):
            view = FullInstrumentViewWindow()
        self.assertFalse(view.is_show_shapes_checkbox_checked())

    def test_on_axes_click_right_calls_presenter_with_right(self):
        event = MagicMock()
        event.inaxes = self._view._detector_spectrum_axes
        event.xdata = 7.0
        event.button = 3
        self._view._on_axes_click(event)
        self._view._presenter.on_peak_selected_in_lineplot.assert_called_once_with(7.0, "right")

    def test_on_axes_click_outside_axes_does_nothing(self):
        event = MagicMock()
        event.inaxes = MagicMock()  # different axes
        event.xdata = 5.0
        self._view._on_axes_click(event)
        self._view._presenter.on_peak_selected_in_lineplot.assert_not_called()

    def test_disable_and_uncheck_selection_list(self):
        mock_item_0 = MagicMock()
        mock_item_0.checkState.return_value = Qt.Checked
        mock_item_1 = MagicMock()
        mock_item_1.checkState.return_value = Qt.Unchecked
        self._view._selection_list = MagicMock()
        self._view._selection_list.count.return_value = 2
        self._view._selection_list.item.side_effect = [mock_item_0, mock_item_1, mock_item_0, mock_item_1]
        self._view._selection_tab = MagicMock()
        self._view.disable_and_uncheck_selection_list()
        mock_item_0.setCheckState.assert_called_with(Qt.Unchecked)
        mock_item_1.setCheckState.assert_called_with(Qt.Unchecked)
        self._view._selection_tab.setEnabled.assert_called_once_with(False)

    def test_enable_and_restore_selection_list(self):
        mock_item_0 = MagicMock()
        mock_item_1 = MagicMock()
        self._view._selection_list = MagicMock()
        self._view._selection_list.count.return_value = 2
        self._view._selection_list.item.side_effect = [mock_item_0, mock_item_1]
        self._view._selection_tab = MagicMock()
        self._view._selection_list_cache = {0: Qt.Checked, 1: Qt.Unchecked}
        self._view.enable_and_restore_selection_list()
        mock_item_0.setCheckState.assert_called_once_with(2)
        mock_item_1.setCheckState.assert_called_once_with(0)
        self._view._selection_tab.setEnabled.assert_called_once_with(True)

    def test_set_delete_all_selected_peaks_button_enabled(self):
        self._view._delete_all_selected_peaks_button = MagicMock()
        self._view.set_delete_all_selected_peaks_button_enabled(False)
        self._view._delete_all_selected_peaks_button.setEnabled.assert_called_once_with(False)


if __name__ == "__main__":
    unittest.main()
