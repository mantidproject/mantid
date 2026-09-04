# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL-3.0+
import unittest
from unittest import mock
from unittest.mock import MagicMock

import numpy as np
from qtpy.QtCore import Qt
from mantidqt.utils.qt.testing import start_qapplication
from mantid.simpleapi import CreateSampleWorkspace
from instrumentview.FullInstrumentViewWindow import FullInstrumentViewView, _LIGHT_GREY
from instrumentview.ShapeWidgets import (
    AnnulusSelectionShape,
    CircleSelectionShape,
    HollowRectangleSelectionShape,
    RectangleSelectionShape,
    EllipseSelectionShape,
)


@start_qapplication
class TestFullInstrumentViewView(unittest.TestCase):
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
            self._view = FullInstrumentViewView()
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

    def test_select_peaks_button_is_checkable(self):
        self.assertEqual(self._view._select_peaks.text(), "Select Peaks")
        self.assertTrue(self._view._select_peaks.isCheckable())

    def test_is_select_bank_tube_checked(self):
        self.assertFalse(self._view.is_select_bank_tube_checked())
        self._view._select_bank_tube.setChecked(True)
        self.assertTrue(self._view.is_select_bank_tube_checked())

    def test_is_select_peaks_checked(self):
        self.assertFalse(self._view.is_select_peaks_checked())
        self._view._select_peaks.setChecked(True)
        self.assertTrue(self._view.is_select_peaks_checked())

    def test_figure_canvas_created(self):
        self._mock_figure_canvas.assert_called_once()

    def test_hover_pick_button_is_checkable(self):
        self.assertTrue(self._view._hover_pick.isCheckable())

    def test_shape_selector_initialised(self):
        self.assertEqual(
            [self._view._shape_selector_combo_box.itemText(i) for i in range(self._view._shape_selector_combo_box.count())],
            ["Circle", "Rectangle", "Ellipse", "Annulus", "Hollow Rectangle"],
        )
        self.assertTrue(self._view._add_shape_button.isCheckable())
        self.assertEqual(self._view._add_shape_button.text(), "Add Shape")

    def test_update_scalar_range(self):
        self._view.set_plotter_scalar_bar_range((0, 100), "label")
        self._view.main_plotter.update_scalar_bar_range.assert_has_calls([mock.call((0, 100), "label")])

    def test_run_on_main_thread_calls_through(self):
        func = MagicMock(return_value="result")
        self.assertEqual(self._view.run_on_main_thread(func, 1, kw=2), "result")
        func.assert_called_once_with(1, kw=2)

    def test_run_on_main_thread_skipped_while_closing(self):
        func = MagicMock()
        self._view._closing = True
        try:
            self.assertIsNone(self._view.run_on_main_thread(func))
            func.assert_not_called()
        finally:
            self._view._closing = False

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

    def test_refresh_peaks_ws_list(self):
        mock_list = MagicMock()
        self._view._peak_ws_list = mock_list
        self._view._presenter.peaks_workspaces_in_ads.return_value = ["existing_ws", "new_ws"]

        self._view.refresh_peaks_ws_list()

        mock_list.refresh_items.assert_called_once_with(["existing_ws", "new_ws"], colours=self._view._COLOURS)

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
        self._view._detector_spectrum_axes.lines = [mock_line]
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
        self.assertIsNotNone(self._view._shape_overlay_manager)
        self.assertIsInstance(self._view._shape_overlay_manager.current_shape, RectangleSelectionShape)

    def test_add_circle_widget(self) -> None:
        self._view.add_circle_widget()
        self.assertIsNotNone(self._view._shape_overlay_manager)
        self.assertIsInstance(self._view._shape_overlay_manager.current_shape, CircleSelectionShape)

    def test_add_ellipse_widget(self) -> None:
        self._view.add_ellipse_widget()
        self.assertIsNotNone(self._view._shape_overlay_manager)
        self.assertIsInstance(self._view._shape_overlay_manager.current_shape, EllipseSelectionShape)

    def test_add_annulus_widget(self) -> None:
        self._view.add_annulus_widget()
        self.assertIsNotNone(self._view._shape_overlay_manager)
        self.assertIsInstance(self._view._shape_overlay_manager.current_shape, AnnulusSelectionShape)

    def test_add_hollow_rectangle_widget(self) -> None:
        self._view.add_hollow_rectangle_widget()
        self.assertIsNotNone(self._view._shape_overlay_manager)
        self.assertIsInstance(self._view._shape_overlay_manager.current_shape, HollowRectangleSelectionShape)

    def test_adding_a_shape_registers_the_live_line_plot_callback(self) -> None:
        self._view.add_circle_widget()
        self.assertEqual(self._view._shape_overlay_manager._on_shape_changed, self._view._presenter.on_shape_changed)

    def test_adding_a_shape_plots_the_spectra_it_covers_straight_away(self) -> None:
        self._view.add_circle_widget()
        self._view._presenter.on_shape_changed.assert_called_once()

    def test_add_selected_shape_uses_dropdown_choice(self) -> None:
        self._view._shape_selector_combo_box.setCurrentText("Ellipse")
        self._view.add_selected_shape(True)
        self.assertIsNotNone(self._view._shape_overlay_manager)
        self.assertIsInstance(self._view._shape_overlay_manager.current_shape, EllipseSelectionShape)

    def test_unchecking_add_shape_button_clears_current_widget(self) -> None:
        self._view._add_shape_button.setChecked(True)
        self._view.add_circle_widget()
        self.assertIsNotNone(self._view._shape_overlay_manager)
        self.assertIsNotNone(self._view._shape_overlay_manager.current_shape)
        self._view.add_selected_shape(False)
        self.assertIsNone(self._view._shape_overlay_manager)

    def test_delete_current_widget(self) -> None:
        self._view.add_circle_widget()
        self.assertIsNotNone(self._view._shape_overlay_manager)
        self._view.delete_current_overlaid_shape()
        self.assertIsNone(self._view._shape_overlay_manager)

    @mock.patch("instrumentview.FullInstrumentViewWindow.ConfigService")
    def test_store_render_mode_option_stores_current_text(self, mock_config):
        self._view._render_mode_combo_box.setCurrentText(self._view._RENDER_MODE_RAW_SHAPES)
        self._view.store_render_mode_option()
        mock_config.Instance.return_value.__setitem__.assert_called_once_with(
            self._view._RENDER_MODE_SETTING_STRING, self._view._RENDER_MODE_RAW_SHAPES
        )

    def test_on_axes_click_left_calls_presenter_with_left(self):
        event = MagicMock()
        event.inaxes = self._view._detector_spectrum_axes
        event.xdata = 5.0
        event.button = 1
        self._view._on_axes_click_during_peak_selection(event)
        self._view._presenter.on_peak_selected_in_lineplot.assert_called_once_with(5.0, "left")

    def test_on_axes_click_zoom_enabled_calls_default_callbacks(self):
        event = MagicMock()
        callback_1 = MagicMock()
        callback_2 = MagicMock()
        self._view._default_lineplot_callbacks = {1: callback_1, 2: callback_2}
        self._view._plot_toolbar.zoom_enabled = MagicMock(return_value=True)
        self._view._plot_toolbar.pan_enabled = MagicMock(return_value=False)

        self._view._on_axes_click_during_peak_selection(event)

        callback_1.assert_called_once_with(event)
        callback_2.assert_called_once_with(event)
        self._view._presenter.on_peak_selected_in_lineplot.assert_not_called()

    def test_on_axes_click_pan_enabled_calls_default_callbacks(self):
        event = MagicMock()
        callback = MagicMock()
        self._view._default_lineplot_callbacks = {1: callback}
        self._view._plot_toolbar.zoom_enabled = MagicMock(return_value=False)
        self._view._plot_toolbar.pan_enabled = MagicMock(return_value=True)

        self._view._on_axes_click_during_peak_selection(event)

        callback.assert_called_once_with(event)
        self._view._presenter.on_peak_selected_in_lineplot.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewWindow.ConfigService")
    def test_get_render_mode_option_returns_current_text(self, mock_config):
        self._view._render_mode_combo_box.setCurrentText(self._view._RENDER_MODE_POINTS)
        self.assertEqual(self._view.get_render_mode_option(), self._view._RENDER_MODE_POINTS)

    @mock.patch("instrumentview.FullInstrumentViewWindow.FigureCanvas")
    @mock.patch("qtpy.QtWidgets.QHBoxLayout.addWidget")
    @mock.patch("qtpy.QtWidgets.QVBoxLayout.addWidget")
    @mock.patch("qtpy.QtWidgets.QSplitter.addWidget")
    @mock.patch("instrumentview.FullInstrumentViewWindow.BackgroundPlotter")
    @mock.patch("instrumentview.FullInstrumentViewWindow.ConfigService")
    def test_render_mode_combo_initialised_to_points_when_config_not_recognised(
        self, mock_config, mock_plotter, mock_splitter, mock_v_layout, mock_h_layout, mock_canvas
    ):
        mock_config.Instance.return_value.__getitem__.return_value = "UnknownValue"
        with mock.patch("mantidqt.utils.qt.qappthreadcall.force_method_calls_to_qapp_thread"):
            view = FullInstrumentViewView()
        self.assertEqual(view.get_render_mode_option(), view._RENDER_MODE_POINTS)

    @mock.patch("instrumentview.FullInstrumentViewWindow.FigureCanvas")
    @mock.patch("qtpy.QtWidgets.QHBoxLayout.addWidget")
    @mock.patch("qtpy.QtWidgets.QVBoxLayout.addWidget")
    @mock.patch("qtpy.QtWidgets.QSplitter.addWidget")
    @mock.patch("instrumentview.FullInstrumentViewWindow.BackgroundPlotter")
    @mock.patch("instrumentview.FullInstrumentViewWindow.ConfigService")
    def test_render_mode_combo_initialised_to_full_shapes_when_config_is_full_shapes(
        self, mock_config, mock_plotter, mock_splitter, mock_v_layout, mock_h_layout, mock_canvas
    ):
        mock_config.Instance.return_value.__getitem__.return_value = self._view._RENDER_MODE_RAW_SHAPES
        with mock.patch("mantidqt.utils.qt.qappthreadcall.force_method_calls_to_qapp_thread"):
            view = FullInstrumentViewView()
        self.assertEqual(view.get_render_mode_option(), self._view._RENDER_MODE_RAW_SHAPES)

    @mock.patch("instrumentview.FullInstrumentViewWindow.FigureCanvas")
    @mock.patch("qtpy.QtWidgets.QHBoxLayout.addWidget")
    @mock.patch("qtpy.QtWidgets.QVBoxLayout.addWidget")
    @mock.patch("qtpy.QtWidgets.QSplitter.addWidget")
    @mock.patch("instrumentview.FullInstrumentViewWindow.BackgroundPlotter")
    @mock.patch("instrumentview.FullInstrumentViewWindow.ConfigService")
    def test_render_mode_combo_initialised_to_shapes_fast_when_config_is_shapes_fast(
        self, mock_config, mock_plotter, mock_splitter, mock_v_layout, mock_h_layout, mock_canvas
    ):
        mock_config.Instance.return_value.__getitem__.return_value = self._view._RENDER_MODE_SHAPES_FAST
        with mock.patch("mantidqt.utils.qt.qappthreadcall.force_method_calls_to_qapp_thread"):
            view = FullInstrumentViewView()
        self.assertEqual(view.get_render_mode_option(), self._view._RENDER_MODE_SHAPES_FAST)

    def test_on_axes_click_right_calls_presenter_with_right(self):
        event = MagicMock()
        event.inaxes = self._view._detector_spectrum_axes
        event.xdata = 7.0
        event.button = 3
        self._view._on_axes_click_during_peak_selection(event)
        self._view._presenter.on_peak_selected_in_lineplot.assert_called_once_with(7.0, "right")

    def test_on_axes_click_outside_axes_does_nothing(self):
        event = MagicMock()
        event.inaxes = MagicMock()  # different axes
        event.xdata = 5.0
        self._view._on_axes_click_during_peak_selection(event)
        self._view._presenter.on_peak_selected_in_lineplot.assert_not_called()

    @mock.patch("instrumentview.FullInstrumentViewWindow.Cursor")
    def test_start_peak_selection_in_lineplot_sets_cursor_disconnects_defaults_and_connects_click_handler(self, mock_cursor):
        callback_1 = MagicMock()
        callback_2 = MagicMock()
        self._view._default_lineplot_callbacks = {11: callback_1, 22: callback_2}
        self._view._detector_figure_canvas.mpl_disconnect = MagicMock()
        self._view._detector_figure_canvas.mpl_connect = MagicMock(return_value=333)

        self._view.start_peak_selection_in_lineplot()

        mock_cursor.assert_called_once_with(self._view._detector_spectrum_axes, color="tab:red", linewidth=1, horizOn=False)
        self._view._detector_figure_canvas.mpl_disconnect.assert_has_calls([mock.call(11), mock.call(22)])
        self._view._detector_figure_canvas.mpl_connect.assert_called_once_with(
            "button_press_event", self._view._on_axes_click_during_peak_selection
        )
        self.assertEqual(self._view._figure_canvas_click_id, 333)
        self.assertIsNotNone(self._view._lineplot_peak_cursor)

    def test_end_peak_selection_in_lineplot_restores_callbacks_and_clears_state(self):
        callback_1 = MagicMock()
        callback_2 = MagicMock()
        self._view._default_lineplot_callbacks = {11: callback_1, 22: callback_2}
        self._view._lineplot_peak_cursor = MagicMock()
        self._view._figure_canvas_click_id = 999
        self._view._detector_figure_canvas.mpl_disconnect = MagicMock()
        self._view._detector_figure_canvas.mpl_connect = MagicMock(side_effect=[111, 222])
        self._view._detector_figure_canvas.draw_idle = MagicMock()

        self._view.end_peak_selection_in_lineplot()

        self._view._detector_figure_canvas.mpl_disconnect.assert_called_once_with(999)
        self._view._detector_figure_canvas.mpl_connect.assert_has_calls(
            [mock.call("button_press_event", callback_1), mock.call("button_press_event", callback_2)]
        )
        self.assertEqual(self._view._default_lineplot_callbacks, {111: callback_1, 222: callback_2})
        self.assertIsNone(self._view._figure_canvas_click_id)
        self.assertIsNone(self._view._lineplot_peak_cursor)
        self._view._detector_figure_canvas.draw_idle.assert_called_once()

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
        mock_item_0.setCheckState.assert_called_once_with(Qt.Checked)
        mock_item_1.setCheckState.assert_called_once_with(Qt.Unchecked)
        self._view._selection_tab.setEnabled.assert_called_once_with(True)

    def test_set_delete_all_selected_peaks_button_enabled(self):
        self._view._delete_all_selected_peaks_button = MagicMock()
        self._view.set_delete_all_selected_peaks_button_enabled(False)
        self._view._delete_all_selected_peaks_button.setEnabled.assert_called_once_with(False)

    def test_set_detector_edit_text_joins_with_semicolon_separator(self):
        mock_edit = MagicMock()
        det_1 = MagicMock()
        det_1.detector_id = 1
        det_2 = MagicMock()
        det_2.detector_id = 2
        self._view._set_detector_edit_text(mock_edit, [det_1, det_2], lambda d: str(d.detector_id))
        mock_edit.setPlainText.assert_called_once_with("1; 2")

    def test_set_detector_edit_text_single_item_no_separator(self):
        mock_edit = MagicMock()
        det = MagicMock()
        det.detector_id = 42
        self._view._set_detector_edit_text(mock_edit, [det], lambda d: str(d.detector_id))
        mock_edit.setPlainText.assert_called_once_with("42")

    def test_create_from_selection_buttons_exist_on_both_tabs(self):
        for button in (self._view._create_selection_from_picked, self._view._create_mask_from_picked):
            self.assertEqual(button.text(), "Create From Current Selection")

    def test_set_create_from_selection_buttons_enabled(self):
        self._view.set_create_from_selection_buttons_enabled(True)
        self.assertTrue(self._view._create_selection_from_picked.isEnabled())
        self.assertTrue(self._view._create_mask_from_picked.isEnabled())

        self._view.set_create_from_selection_buttons_enabled(False)
        self.assertFalse(self._view._create_selection_from_picked.isEnabled())
        self.assertFalse(self._view._create_mask_from_picked.isEnabled())

    def test_create_from_selection_buttons_notify_presenter(self):
        self._view.setup_connections_to_presenter()
        # Connecting leaves them disabled until the presenter reports a selection
        self.assertFalse(self._view._create_selection_from_picked.isEnabled())
        self._view.set_create_from_selection_buttons_enabled(True)

        self._view._create_selection_from_picked.click()
        self._view._create_mask_from_picked.click()

        self.assertEqual(self._view._presenter.on_create_item_from_selection_clicked.call_count, 2)

    def test_on_show_monitors_toggled_sets_presenter_color_when_checked(self):
        self._view._presenter.monitor_colour = (230, 55, 55)
        with mock.patch.object(self._view._show_monitors_check_box, "set_colour") as mock_set_colour:
            self._view._on_show_monitors_toggled(True)
        mock_set_colour.assert_called_once_with((230, 55, 55))

    def test_on_show_monitors_toggled_uses_grey_when_unchecked(self):
        self._view._presenter.monitor_colour = (230, 55, 55)
        with mock.patch.object(self._view._show_monitors_check_box, "set_colour") as mock_set_colour:
            self._view._on_show_monitors_toggled(False)
        mock_set_colour.assert_called_once_with(_LIGHT_GREY)

    def test_on_show_sample_position_toggled_sets_presenter_color_when_checked(self):
        self._view._presenter.sample_position_colour = (70, 160, 70)
        with mock.patch.object(self._view._show_sample_position_check_box, "set_colour") as mock_set_colour:
            self._view._on_show_sample_position_toggled(True)
        mock_set_colour.assert_called_once_with((70, 160, 70))

    def test_on_show_sample_position_toggled_uses_grey_when_unchecked(self):
        self._view._presenter.sample_position_colour = (70, 160, 70)
        with mock.patch.object(self._view._show_sample_position_check_box, "set_colour") as mock_set_colour:
            self._view._on_show_sample_position_toggled(False)
        mock_set_colour.assert_called_once_with(_LIGHT_GREY)


if __name__ == "__main__":
    unittest.main()
