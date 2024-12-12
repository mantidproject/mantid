# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from unittest.mock import MagicMock, call, patch

from workbench.widgets.settings.plots.model import PlotsSettingsModel, PlotProperties
from workbench.widgets.settings.test_utilities.settings_model_test_base import BaseSettingsModelTest


class MockFontProperty:
    def __init__(self, font: str):
        self.font = font

    def get_name(self):
        return self.font


class MockFontManager:
    def __init__(self):
        def construct_mock_font_property(fname: str):
            return MockFontProperty(fname)

        self.findSystemFonts = MagicMock()
        self.FontProperties = MagicMock(side_effect=construct_mock_font_property)


class PlotsSettingsModelTest(BaseSettingsModelTest):
    GET_SAVED_VALUE_PATCH_PATH = "workbench.widgets.settings.plots.model.PlotsSettingsModel.get_saved_value"
    ADD_CHANGE_PATCH_PATH = "workbench.widgets.settings.plots.model.PlotsSettingsModel.add_change"

    def setUp(self) -> None:
        self.model = PlotsSettingsModel()

    @patch("workbench.widgets.settings.plots.model.PlotsSettingsModel.get_current_mpl_font")
    @patch("workbench.widgets.settings.plots.model.font_manager", new_callable=MockFontManager)
    def test_get_font_names(self, mock_font_manager: MockFontManager, mock_get_current_mpl_font: MagicMock):
        mock_get_current_mpl_font.return_value = "roboto"
        fonts = ["calibri", "helvetica", "times new roman", "computer modern"]
        mock_font_manager.findSystemFonts.return_value = fonts
        font_names_from_model = self.model.get_font_names()
        self.assertEqual(sorted(fonts + ["roboto"]), font_names_from_model)

    @patch("workbench.widgets.settings.plots.model.PlotsSettingsModel.get_current_mpl_font")
    @patch("workbench.widgets.settings.plots.model.font_manager", new_callable=MockFontManager)
    def test_get_font_names_doesnt_duplicate_fonts(self, mock_font_manager: MockFontManager, mock_get_current_mpl_font: MagicMock):
        mock_get_current_mpl_font.return_value = "calibri"
        fonts = ["calibri", "helvetica", "times new roman", "computer modern"]
        mock_font_manager.findSystemFonts.return_value = fonts
        font_names_from_model = self.model.get_font_names()
        self.assertEqual(sorted(fonts), font_names_from_model)

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_normalize_to_bin_width(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_normalize_to_bin_width, ["On", "Off"], call(PlotProperties.NORMALIZATION.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_show_title(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_show_title, ["On", "Off"], call(PlotProperties.SHOW_TITLE.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_show_legend(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_show_legend, ["On", "Off"], call(PlotProperties.SHOW_LEGEND.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_x_axes_scale(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_x_axes_scale, ["Linear", "Log"], call(PlotProperties.X_AXES_SCALE.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_y_axes_scale(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_y_axes_scale, ["Linear", "Log"], call(PlotProperties.Y_AXES_SCALE.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_axes_line_width(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_axes_line_width, ["10", "1.60"], call(PlotProperties.AXES_LINE_WIDTH.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_x_min(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(get_saved_value_mock, self.model.get_x_min, ["5", "-5"], call(PlotProperties.X_MIN.value))

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_x_max(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(get_saved_value_mock, self.model.get_x_max, ["5", "-5"], call(PlotProperties.X_MAX.value))

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_y_min(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(get_saved_value_mock, self.model.get_y_min, ["5", "-5"], call(PlotProperties.Y_MIN.value))

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_y_max(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(get_saved_value_mock, self.model.get_y_max, ["5", "-5"], call(PlotProperties.Y_MAX.value))

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_show_ticks_left(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_show_ticks_left, ["On", "Off"], call(PlotProperties.SHOW_TICKS_LEFT.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_show_ticks_bottom(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_show_ticks_bottom, ["On", "Off"], call(PlotProperties.SHOW_TICKS_BOTTOM.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_show_ticks_right(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_show_ticks_right, ["On", "Off"], call(PlotProperties.SHOW_TICKS_RIGHT.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_show_ticks_top(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_show_ticks_top, ["On", "Off"], call(PlotProperties.SHOW_TICKS_TOP.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_show_labels_left(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_show_labels_left, ["On", "Off"], call(PlotProperties.SHOW_LABELS_LEFT.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_show_labels_bottom(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_show_labels_bottom, ["On", "Off"], call(PlotProperties.SHOW_LABELS_BOTTOM.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_show_labels_right(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_show_labels_right, ["On", "Off"], call(PlotProperties.SHOW_LABELS_RIGHT.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_show_labels_top(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_show_labels_top, ["On", "Off"], call(PlotProperties.SHOW_LABELS_TOP.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_major_ticks_length(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_major_ticks_length, ["12", "0.12"], call(PlotProperties.MAJOR_TICKS_LENGTH.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_major_ticks_width(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_major_ticks_width, ["12", "0.12"], call(PlotProperties.MAJOR_TICKS_WIDTH.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_major_ticks_direction(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_major_ticks_direction, ["In", "Out"], call(PlotProperties.MAJOR_TICKS_DIRECTION.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_minor_ticks_length(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_minor_ticks_length, ["12", "0.12"], call(PlotProperties.MINOR_TICKS_LENGTH.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_minor_ticks_width(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_minor_ticks_width, ["12", "0.12"], call(PlotProperties.MINOR_TICKS_WIDTH.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_minor_ticks_direction(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_minor_ticks_direction, ["In", "Out"], call(PlotProperties.MINOR_TICKS_DIRECTION.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_enable_grid(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_enable_grid, ["On", "Off"], call(PlotProperties.ENABLE_GRID.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_show_minor_ticks(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_show_minor_ticks, ["On", "Off"], call(PlotProperties.SHOW_MINOR_TICKS.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_show_minor_gridlines(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_show_minor_gridlines, ["On", "Off"], call(PlotProperties.SHOW_MINOR_GRIDLINES.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_plot_font(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_plot_font, ["calibri", "roboto"], call(PlotProperties.PLOT_FONT.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_line_style(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_line_style, ["solid", "dashed"], call(PlotProperties.LINE_STYLE.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_draw_style(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_draw_style, ["default", "steps"], call(PlotProperties.DRAW_STYLE.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_line_width(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_line_width, ["10", "0.2"], call(PlotProperties.LINE_WIDTH.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_marker_style(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_marker_style, ["point", "octagon"], call(PlotProperties.MARKER_STYLE.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_marker_size(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_marker_size, ["0.4", "12"], call(PlotProperties.MARKER_SIZE.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_error_width(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_error_width, ["0.4", "12"], call(PlotProperties.ERROR_WIDTH.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_capsize(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_capsize, ["0.4", "12"], call(PlotProperties.CAPSIZE.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_cap_thickness(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_cap_thickness, ["0.4", "12"], call(PlotProperties.CAP_THICKNESS.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_error_every(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_error_every, ["5", "10"], call(PlotProperties.ERROR_EVERY.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_legend_location(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock,
            self.model.get_legend_location,
            ["upper right", "center right"],
            call(PlotProperties.LEGEND_LOCATION.value),
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_legend_font_size(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_legend_font_size, ["5", "10"], call(PlotProperties.LEGEND_FONT_SIZE.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_color_map(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_color_map, ["jet", "viridis"], call(PlotProperties.COLORMAP.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_colorbar_scale(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_colorbar_scale, ["Log", "Linear"], call(PlotProperties.COLORBAR_SCALE.value)
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_normalize_by_bin_width(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_normalize_by_bin_width, ["Off", "On"], PlotProperties.NORMALIZATION.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_show_title(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_show_title, ["Off", "On"], PlotProperties.SHOW_TITLE.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_enable_grid(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_enable_grid, ["Off", "On"], PlotProperties.ENABLE_GRID.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_show_minor_ticks(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_show_minor_ticks, ["Off", "On"], PlotProperties.SHOW_MINOR_TICKS.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_show_minor_gridlines(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_show_minor_gridlines, ["Off", "On"], PlotProperties.SHOW_MINOR_GRIDLINES.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_plot_font(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_plot_font, ["time new roman", "computer modern"], PlotProperties.PLOT_FONT.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_show_legend(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_show_legend, ["Off", "On"], PlotProperties.SHOW_LEGEND.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_x_axes_scale(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_x_axes_scale, ["Linear", "Log"], PlotProperties.X_AXES_SCALE.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_y_axes_scale(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_y_axes_scale, ["Linear", "Log"], PlotProperties.Y_AXES_SCALE.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_axes_line_width(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_axes_line_width, ["10", "5.7"], PlotProperties.AXES_LINE_WIDTH.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_show_ticks_left(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_show_ticks_left, ["Off", "On"], PlotProperties.SHOW_TICKS_LEFT.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_show_ticks_bottom(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_show_ticks_bottom, ["Off", "On"], PlotProperties.SHOW_TICKS_BOTTOM.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_show_ticks_right(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_show_ticks_right, ["Off", "On"], PlotProperties.SHOW_TICKS_RIGHT.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_show_ticks_top(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_show_ticks_top, ["Off", "On"], PlotProperties.SHOW_TICKS_TOP.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_show_labels_left(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_show_labels_left, ["Off", "On"], PlotProperties.SHOW_LABELS_LEFT.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_show_labels_bottom(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_show_labels_bottom, ["Off", "On"], PlotProperties.SHOW_LABELS_BOTTOM.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_show_labels_right(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_show_labels_right, ["Off", "On"], PlotProperties.SHOW_LABELS_RIGHT.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_show_labels_top(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_show_labels_top, ["Off", "On"], PlotProperties.SHOW_LABELS_TOP.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_major_ticks_length(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_major_ticks_length, ["3", "4.1"], PlotProperties.MAJOR_TICKS_LENGTH.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_major_ticks_width(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_major_ticks_width, ["3", "4.1"], PlotProperties.MAJOR_TICKS_WIDTH.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_major_ticks_direction(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_major_ticks_direction, ["In", "Out"], PlotProperties.MAJOR_TICKS_DIRECTION.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_minor_ticks_length(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_minor_ticks_length, ["3", "4.1"], PlotProperties.MINOR_TICKS_LENGTH.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_minor_ticks_width(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_minor_ticks_width, ["3", "4.1"], PlotProperties.MINOR_TICKS_WIDTH.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_minor_ticks_direction(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_minor_ticks_direction, ["In", "Out"], PlotProperties.MINOR_TICKS_DIRECTION.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_line_style(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_line_style, ["solid", "dashed"], PlotProperties.LINE_STYLE.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_draw_style(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_draw_style, ["default", "steps"], PlotProperties.DRAW_STYLE.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_line_width(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(add_change_mock, self.model.set_line_width, ["3", "4.1"], PlotProperties.LINE_WIDTH.value)

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_x_min(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(add_change_mock, self.model.set_x_min, ["5", "-3.8"], PlotProperties.X_MIN.value)

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_x_max(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(add_change_mock, self.model.set_x_max, ["5", "-3.8"], PlotProperties.X_MAX.value)

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_y_min(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(add_change_mock, self.model.set_y_min, ["5", "-3.8"], PlotProperties.Y_MIN.value)

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_y_max(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(add_change_mock, self.model.set_y_max, ["5", "-3.8"], PlotProperties.Y_MAX.value)

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_marker_style(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_marker_style, ["point", "plus"], PlotProperties.MARKER_STYLE.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_error_width(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_error_width, ["5", "20"], PlotProperties.ERROR_WIDTH.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_capsize(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(add_change_mock, self.model.set_capsize, ["5", "20"], PlotProperties.CAPSIZE.value)

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_cap_thickness(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_cap_thickness, ["5", "20"], PlotProperties.CAP_THICKNESS.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_error_every(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_error_every, ["5", "20"], PlotProperties.ERROR_EVERY.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_legend_location(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_legend_location, ["upper left", "centre right"], PlotProperties.LEGEND_LOCATION.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_legend_font_size(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_legend_font_size, ["5", "20"], PlotProperties.LEGEND_FONT_SIZE.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_colorbar_scale(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_colorbar_scale, ["Log", "Linear"], PlotProperties.COLORBAR_SCALE.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_color_map(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_color_map, ["jet_r", "greyscale"], PlotProperties.COLORMAP.value
        )
