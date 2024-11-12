# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import MagicMock, patch
from mantidqt.utils.qt.testing import start_qapplication
from workbench.widgets.settings.plots.presenter import PlotSettings
from workbench.widgets.settings.test_utilities.settings_test_utilities import (
    assert_presenter_has_added_mousewheel_filter_to_all_como_and_spin_boxes,
)

from qtpy.QtCore import Qt


class MockPlotsSettingsModel:
    def __init__(self):
        self.get_font_names = MagicMock()
        self.get_normalize_to_bin_width = MagicMock()
        self.get_show_title = MagicMock()
        self.get_show_legend = MagicMock()
        self.get_x_axes_scale = MagicMock()
        self.get_y_axes_scale = MagicMock()
        self.get_axes_line_width = MagicMock()
        self.get_x_min = MagicMock()
        self.get_x_max = MagicMock()
        self.get_y_min = MagicMock()
        self.get_y_max = MagicMock()
        self.get_show_ticks_left = MagicMock()
        self.get_show_ticks_bottom = MagicMock()
        self.get_show_ticks_right = MagicMock()
        self.get_show_ticks_top = MagicMock()
        self.get_show_labels_left = MagicMock()
        self.get_show_labels_bottom = MagicMock()
        self.get_show_labels_right = MagicMock()
        self.get_show_labels_top = MagicMock()
        self.get_major_ticks_length = MagicMock()
        self.get_major_ticks_width = MagicMock()
        self.get_major_ticks_direction = MagicMock()
        self.get_minor_ticks_length = MagicMock()
        self.get_minor_ticks_width = MagicMock()
        self.get_minor_ticks_direction = MagicMock()
        self.get_enable_grid = MagicMock()
        self.get_show_minor_ticks = MagicMock()
        self.get_show_minor_gridlines = MagicMock()
        self.get_plot_font = MagicMock(return_value="My Cool Font")
        self.get_line_style = MagicMock()
        self.get_draw_style = MagicMock()
        self.get_line_width = MagicMock()
        self.get_marker_style = MagicMock()
        self.get_marker_size = MagicMock()
        self.get_error_width = MagicMock()
        self.get_capsize = MagicMock()
        self.get_cap_thickness = MagicMock()
        self.get_error_every = MagicMock()
        self.get_legend_location = MagicMock()
        self.get_legend_font_size = MagicMock()
        self.get_color_map = MagicMock(return_value="jet")
        self.get_colorbar_scale = MagicMock()
        self.set_normalize_by_bin_width = MagicMock()
        self.set_show_title = MagicMock()
        self.set_x_axes_scale = MagicMock()
        self.set_y_axes_scale = MagicMock()
        self.set_axes_line_width = MagicMock()
        self.set_x_min = MagicMock()
        self.set_x_max = MagicMock()
        self.set_y_min = MagicMock()
        self.set_y_max = MagicMock()
        self.set_enable_grid = MagicMock()
        self.set_show_ticks_left = MagicMock()
        self.set_show_ticks_bottom = MagicMock()
        self.set_show_ticks_right = MagicMock()
        self.set_show_ticks_top = MagicMock()
        self.set_show_labels_left = MagicMock()
        self.set_show_labels_bottom = MagicMock()
        self.set_show_labels_right = MagicMock()
        self.set_show_labels_top = MagicMock()
        self.set_line_style = MagicMock()
        self.set_line_width = MagicMock()
        self.set_marker_style = MagicMock()
        self.set_marker_size = MagicMock()
        self.set_error_width = MagicMock()
        self.set_capsize = MagicMock()
        self.set_cap_thickness = MagicMock()
        self.set_error_every = MagicMock()
        self.set_show_minor_ticks = MagicMock()
        self.set_show_minor_gridlines = MagicMock()
        self.set_show_legend = MagicMock()
        self.set_legend_location = MagicMock()
        self.set_legend_font_size = MagicMock()
        self.set_color_map = MagicMock()
        self.set_plot_font = MagicMock()


@start_qapplication
class PlotsSettingsTest(unittest.TestCase):
    MOUSEWHEEL_EVENT_FILTER_PATH = "workbench.widgets.settings.plots.presenter.filter_out_mousewheel_events_from_combo_or_spin_box"

    def setUp(self) -> None:
        self.mock_model = MockPlotsSettingsModel()

    @patch(MOUSEWHEEL_EVENT_FILTER_PATH)
    def test_filters_added_to_combo_and_spin_boxes(self, mock_mousewheel_filter):
        presenter = PlotSettings(None, model=self.mock_model)
        view = presenter.get_view()
        assert_presenter_has_added_mousewheel_filter_to_all_como_and_spin_boxes(view, mock_mousewheel_filter)

    def test_load_current_setting_values(self):
        # load current setting is called automatically in the constructor
        PlotSettings(None, model=self.mock_model)

        self.mock_model.get_normalize_to_bin_width.assert_called_once()
        self.mock_model.get_show_title.assert_called_once()
        self.mock_model.get_show_legend.assert_called_once()
        self.mock_model.get_plot_font.assert_called_once()
        self.mock_model.get_x_axes_scale.assert_called_once()
        self.mock_model.get_y_axes_scale.assert_called_once()
        self.mock_model.get_x_min.assert_called_once()
        self.mock_model.get_x_max.assert_called_once()
        self.mock_model.get_y_min.assert_called_once()
        self.mock_model.get_y_max.assert_called_once()
        self.mock_model.get_show_ticks_left.assert_called_once()
        self.mock_model.get_show_ticks_bottom.assert_called_once()
        self.mock_model.get_show_ticks_right.assert_called_once()
        self.mock_model.get_show_ticks_top.assert_called_once()
        self.mock_model.get_show_labels_left.assert_called_once()
        self.mock_model.get_show_labels_bottom.assert_called_once()
        self.mock_model.get_show_labels_right.assert_called_once()
        self.mock_model.get_show_labels_top.assert_called_once()
        self.mock_model.get_major_ticks_length.assert_called_once()
        self.mock_model.get_major_ticks_width.assert_called_once()
        self.mock_model.get_major_ticks_direction.assert_called_once()
        self.mock_model.get_minor_ticks_length.assert_called_once()
        self.mock_model.get_minor_ticks_width.assert_called_once()
        self.mock_model.get_minor_ticks_direction.assert_called_once()
        self.mock_model.get_enable_grid.assert_called_once()
        self.mock_model.get_show_minor_ticks.assert_called_once()
        self.mock_model.get_show_minor_gridlines.assert_called_once()
        self.mock_model.get_line_style.assert_called_once()
        self.mock_model.get_draw_style.assert_called_once()
        self.mock_model.get_line_width.assert_called_once()
        self.mock_model.get_marker_style.assert_called_once()
        self.mock_model.get_marker_size.assert_called_once()
        self.mock_model.get_error_width.assert_called_once()
        self.mock_model.get_capsize.assert_called_once()
        self.mock_model.get_cap_thickness.assert_called_once()
        self.mock_model.get_error_every.assert_called_once()
        self.mock_model.get_legend_location.assert_called_once()
        self.mock_model.get_legend_font_size.assert_called_once()
        self.mock_model.get_color_map.assert_called_once()

    def test_action_normalization_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_normalization_changed(Qt.Checked)
        self.mock_model.set_normalize_by_bin_width.assert_called_once_with("On")

        self.mock_model.set_normalize_by_bin_width.reset_mock()

        presenter.action_normalization_changed(Qt.Unchecked)
        self.mock_model.set_normalize_by_bin_width.assert_called_once_with("Off")

    def test_action_show_title_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_show_title_changed(Qt.Checked)
        self.mock_model.set_show_title.assert_called_once_with("On")

        self.mock_model.set_show_title.reset_mock()

        presenter.action_show_title_changed(Qt.Unchecked)
        self.mock_model.set_show_title.assert_called_once_with("Off")

    def test_action_default_x_axes_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_default_x_axes_changed("Linear")
        self.mock_model.set_x_axes_scale.assert_called_once_with("Linear")

        self.mock_model.set_x_axes_scale.reset_mock()

        presenter.action_default_x_axes_changed("Log")
        self.mock_model.set_x_axes_scale.assert_called_once_with("Log")

    def test_action_default_y_axes_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_default_y_axes_changed("Linear")
        self.mock_model.set_y_axes_scale.assert_called_once_with("Linear")

        self.mock_model.set_y_axes_scale.reset_mock()

        presenter.action_default_y_axes_changed("Log")
        self.mock_model.set_y_axes_scale.assert_called_once_with("Log")

    def test_action_axes_line_width_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_axes_line_width_changed(2)
        self.mock_model.set_axes_line_width.assert_called_once_with("2")

        self.mock_model.set_axes_line_width.reset_mock()

        presenter.action_axes_line_width_changed(3.5)
        self.mock_model.set_axes_line_width.assert_called_once_with("3.5")

    def test_action_x_min_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_x_min_changed(3.2)
        self.mock_model.set_x_min.assert_called_once_with("3.2")

        self.mock_model.set_x_min.reset_mock()

        presenter.action_x_min_changed(1.5)
        self.mock_model.set_x_min.assert_called_once_with("1.5")

    def test_action_x_max_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_x_max_changed(3.2)
        self.mock_model.set_x_max.assert_called_once_with("3.2")

        self.mock_model.set_x_max.reset_mock()

        presenter.action_x_max_changed(1.5)
        self.mock_model.set_x_max.assert_called_once_with("1.5")

    def test_action_y_min_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_y_min_changed(3.2)
        self.mock_model.set_y_min.assert_called_once_with("3.2")

        self.mock_model.set_y_min.reset_mock()

        presenter.action_y_min_changed(1.5)
        self.mock_model.set_y_min.assert_called_once_with("1.5")

    def test_action_y_max_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_y_max_changed(3.2)
        self.mock_model.set_y_max.assert_called_once_with("3.2")

        self.mock_model.set_y_max.reset_mock()

        presenter.action_y_max_changed(1.5)
        self.mock_model.set_y_max.assert_called_once_with("1.5")

    def test_x_min_box_and_check_box_disabled_if_no_value(self):
        self.mock_model.get_x_min.return_value = ""

        presenter = PlotSettings(None, model=self.mock_model)

        self.assertFalse(presenter.get_view().x_min.isEnabled())
        self.assertFalse(presenter.get_view().x_min_box.isChecked())

    def test_x_max_box_and_check_box_disabled_if_no_value(self):
        self.mock_model.get_x_max.return_value = ""

        presenter = PlotSettings(None, model=self.mock_model)

        self.assertFalse(presenter.get_view().x_max.isEnabled())
        self.assertFalse(presenter.get_view().x_max_box.isChecked())

    def test_y_min_box_and_check_box_disabled_if_no_value(self):
        self.mock_model.get_y_min.return_value = ""

        presenter = PlotSettings(None, model=self.mock_model)

        self.assertFalse(presenter.get_view().y_min.isEnabled())
        self.assertFalse(presenter.get_view().y_min_box.isChecked())

    def test_y_max_box_and_check_box_disabled_if_no_value(self):
        self.mock_model.get_y_max.return_value = ""

        presenter = PlotSettings(None, model=self.mock_model)

        self.assertFalse(presenter.get_view().y_max.isEnabled())
        self.assertFalse(presenter.get_view().y_max_box.isChecked())

    def test_x_min_box_and_check_box_enabled_if_value(self):
        self.mock_model.get_x_min.return_value = "50"

        presenter = PlotSettings(None, model=self.mock_model)

        self.assertTrue(presenter.get_view().x_min.isEnabled())
        self.assertTrue(presenter.get_view().x_min_box.isChecked())

    def test_x_max_box_and_check_box_enabled_if_value(self):
        self.mock_model.get_x_max.return_value = "50"

        presenter = PlotSettings(None, model=self.mock_model)

        self.assertTrue(presenter.get_view().x_max.isEnabled())
        self.assertTrue(presenter.get_view().x_max_box.isChecked())

    def test_y_min_box_and_check_box_enabled_if_value(self):
        self.mock_model.get_y_min.return_value = "50"

        presenter = PlotSettings(None, model=self.mock_model)

        self.assertTrue(presenter.get_view().y_min.isEnabled())
        self.assertTrue(presenter.get_view().y_min_box.isChecked())

    def test_y_max_box_and_check_box_enabled_if_value(self):
        self.mock_model.get_y_max.return_value = "50"

        presenter = PlotSettings(None, model=self.mock_model)

        self.assertTrue(presenter.get_view().y_max.isEnabled())
        self.assertTrue(presenter.get_view().y_max_box.isChecked())

    def test_x_min_checkbox_clears_property_when_disabled(self):
        self.mock_model.get_x_min.return_value = "50"

        presenter = PlotSettings(None, model=self.mock_model)
        presenter.action_x_min_box_changed(False)

        self.mock_model.set_x_min.assert_called_once_with("")

    def test_x_max_checkbox_clears_property_when_disabled(self):
        self.mock_model.get_x_max.return_value = "50"

        presenter = PlotSettings(None, model=self.mock_model)
        presenter.action_x_max_box_changed(False)

        self.mock_model.set_x_max.assert_called_once_with("")

    def test_y_min_checkbox_clears_property_when_disabled(self):
        self.mock_model.get_y_min.return_value = "50"

        presenter = PlotSettings(None, model=self.mock_model)
        presenter.action_y_min_box_changed(False)

        self.mock_model.set_y_min.assert_called_once_with("")

    def test_y_max_checkbox_clears_property_when_disabled(self):
        self.mock_model.get_y_max.return_value = "50"

        presenter = PlotSettings(None, model=self.mock_model)
        presenter.action_y_max_box_changed(False)

        self.mock_model.set_y_max.assert_called_once_with("")

    def test_action_enable_grid(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_enable_grid_changed(Qt.Checked)
        self.mock_model.set_enable_grid.assert_called_once_with("On")

        self.mock_model.set_enable_grid.reset_mock()

        presenter.action_enable_grid_changed(Qt.Unchecked)
        self.mock_model.set_enable_grid.assert_called_once_with("Off")

    def test_action_show_ticks_left(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_show_ticks_left_changed(Qt.Checked)
        self.mock_model.set_show_ticks_left.assert_called_once_with("On")

        self.mock_model.set_show_ticks_left.reset_mock()

        presenter.action_show_ticks_left_changed(Qt.Unchecked)
        self.mock_model.set_show_ticks_left.assert_called_once_with("Off")

    def test_action_show_ticks_bottom(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_show_ticks_bottom_changed(Qt.Checked)
        self.mock_model.set_show_ticks_bottom.assert_called_once_with("On")

        self.mock_model.set_show_ticks_bottom.reset_mock()

        presenter.action_show_ticks_bottom_changed(Qt.Unchecked)
        self.mock_model.set_show_ticks_bottom.assert_called_once_with("Off")

    def test_action_show_ticks_right(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_show_ticks_right_changed(Qt.Checked)
        self.mock_model.set_show_ticks_right.assert_called_once_with("On")

        self.mock_model.set_show_ticks_right.reset_mock()

        presenter.action_show_ticks_right_changed(Qt.Unchecked)
        self.mock_model.set_show_ticks_right.assert_called_once_with("Off")

    def test_action_show_ticks_top(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_show_ticks_top_changed(Qt.Checked)
        self.mock_model.set_show_ticks_top.assert_called_once_with("On")

        self.mock_model.set_show_ticks_top.reset_mock()

        presenter.action_show_ticks_top_changed(Qt.Unchecked)
        self.mock_model.set_show_ticks_top.assert_called_once_with("Off")

    def test_action_show_labels_left(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_show_labels_left_changed(Qt.Checked)
        self.mock_model.set_show_labels_left.assert_called_once_with("On")

        self.mock_model.set_show_labels_left.reset_mock()

        presenter.action_show_labels_left_changed(Qt.Unchecked)
        self.mock_model.set_show_labels_left.assert_called_once_with("Off")

    def test_action_show_labels_bottom(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_show_labels_bottom_changed(Qt.Checked)
        self.mock_model.set_show_labels_bottom.assert_called_once_with("On")

        self.mock_model.set_show_labels_bottom.reset_mock()

        presenter.action_show_labels_bottom_changed(Qt.Unchecked)
        self.mock_model.set_show_labels_bottom.assert_called_once_with("Off")

    def test_action_show_labels_right(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_show_labels_right_changed(Qt.Checked)
        self.mock_model.set_show_labels_right.assert_called_once_with("On")

        self.mock_model.set_show_labels_right.reset_mock()

        presenter.action_show_labels_right_changed(Qt.Unchecked)
        self.mock_model.set_show_labels_right.assert_called_once_with("Off")

    def test_action_show_labels_top(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_show_labels_top_changed(Qt.Checked)
        self.mock_model.set_show_labels_top.assert_called_once_with("On")

        self.mock_model.set_show_labels_top.reset_mock()

        presenter.action_show_labels_top_changed(Qt.Unchecked)
        self.mock_model.set_show_labels_top.assert_called_once_with("Off")

    def test_action_line_style_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_line_style_changed("dashed")
        self.mock_model.set_line_style.assert_called_once_with("dashed")

        self.mock_model.set_line_style.reset_mock()

        presenter.action_line_style_changed("dotted")
        self.mock_model.set_line_style.assert_called_once_with("dotted")

    def test_action_line_width_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_line_width_changed(2)
        self.mock_model.set_line_width.assert_called_once_with("2")

        self.mock_model.set_line_width.reset_mock()

        presenter.action_line_width_changed(3.5)
        self.mock_model.set_line_width.assert_called_once_with("3.5")

    def test_action_marker_style_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_marker_style_changed("circle")
        self.mock_model.set_marker_style.assert_called_once_with("circle")

        self.mock_model.set_marker_style.reset_mock()

        presenter.action_marker_style_changed("octagon")
        self.mock_model.set_marker_style.assert_called_once_with("octagon")

    def test_action_marker_size_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_marker_size_changed("8.0")
        self.mock_model.set_marker_size.assert_called_once_with("8.0")

        self.mock_model.set_marker_size.reset_mock()

        presenter.action_marker_size_changed("5.0")
        self.mock_model.set_marker_size.assert_called_once_with("5.0")

    def test_action_error_width_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_error_width_changed(2)
        self.mock_model.set_error_width.assert_called_once_with("2")

        self.mock_model.set_error_width.reset_mock()

        presenter.action_error_width_changed(1.5)
        self.mock_model.set_error_width.assert_called_once_with("1.5")

    def test_action_capsize_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_capsize_changed(2)
        self.mock_model.set_capsize.assert_called_once_with("2")

        self.mock_model.set_capsize.reset_mock()

        presenter.action_capsize_changed(1.5)
        self.mock_model.set_capsize.assert_called_once_with("1.5")

    def test_action_cap_thickness_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_cap_thickness_changed(2)
        self.mock_model.set_cap_thickness.assert_called_once_with("2")

        self.mock_model.set_cap_thickness.reset_mock()

        presenter.action_cap_thickness_changed(1.5)
        self.mock_model.set_cap_thickness.assert_called_once_with("1.5")

    def test_action_error_every_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_error_every_changed(2)
        self.mock_model.set_error_every.assert_called_once_with("2")

        self.mock_model.set_error_every.reset_mock()

        presenter.action_error_every_changed(5)
        self.mock_model.set_error_every.assert_called_once_with("5")

    def test_action_show_minor_ticks_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_show_minor_ticks_changed(Qt.Checked)
        self.mock_model.set_show_minor_ticks.assert_called_once_with("On")

        self.mock_model.set_show_minor_ticks.reset_mock()

        presenter.action_show_minor_ticks_changed(Qt.Unchecked)
        self.mock_model.set_show_minor_ticks.assert_called_once_with("Off")

    def test_action_show_minor_gridlines_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_show_minor_gridlines_changed(Qt.Checked)
        self.mock_model.set_show_minor_gridlines.assert_called_once_with("On")

        self.mock_model.set_show_minor_gridlines.reset_mock()

        presenter.action_show_minor_gridlines_changed(Qt.Unchecked)
        self.mock_model.set_show_minor_gridlines.assert_called_once_with("Off")

    def test_action_show_legend_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_show_legend_changed(Qt.Checked)
        self.mock_model.set_show_legend.assert_called_once_with("On")

        self.mock_model.set_show_legend.reset_mock()

        presenter.action_show_legend_changed(Qt.Unchecked)
        self.mock_model.set_show_legend.assert_called_once_with("Off")

    def test_action_legend_every_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_legend_location_changed("best")
        self.mock_model.set_legend_location.assert_called_once_with("best")

        self.mock_model.set_legend_location.reset_mock()

        presenter.action_legend_location_changed("upper left")
        self.mock_model.set_legend_location.assert_called_once_with("upper left")

    def test_action_legend_size_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_legend_size_changed(10)
        self.mock_model.set_legend_font_size.assert_called_once_with("10")

        self.mock_model.set_legend_font_size.reset_mock()

        presenter.action_legend_size_changed(8)
        self.mock_model.set_legend_font_size.assert_called_once_with("8")

    def test_action_default_colormap_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)
        presenter.get_view().default_colormap_combo_box.setCurrentIndex(4)
        colormap = presenter.get_view().default_colormap_combo_box.currentText()

        self.mock_model.set_color_map.reset_mock()

        presenter.action_default_colormap_changed()
        self.mock_model.set_color_map.assert_called_once_with(colormap)

        presenter.get_view().default_colormap_combo_box.setCurrentIndex(5)
        colormap = presenter.get_view().default_colormap_combo_box.currentText()
        presenter.get_view().reverse_colormap_check_box.setChecked(True)

        self.mock_model.set_color_map.reset_mock()

        presenter.action_default_colormap_changed()
        self.mock_model.set_color_map.assert_called_once_with(colormap + "_r")

    def test_action_font_combo_changed(self):
        presenter = PlotSettings(None, model=self.mock_model)

        presenter.action_font_combo_changed("Helvetica")
        self.mock_model.set_plot_font.assert_called_once_with("Helvetica")

        self.mock_model.set_plot_font.reset_mock()

        presenter.action_font_combo_changed("Something that is not a font")
        self.mock_model.set_plot_font.assert_called_once_with("Something that is not a font")


if __name__ == "__main__":
    unittest.main()
