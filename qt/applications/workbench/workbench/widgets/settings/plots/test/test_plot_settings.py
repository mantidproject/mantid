# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
import unittest

from unittest.mock import call, patch
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.testing.strict_mock import StrictMock
from workbench.widgets.settings.plots.presenter import PlotSettings, PlotProperties

from qtpy.QtCore import Qt


class MockConfigService(object):
    def __init__(self):
        self.getString = StrictMock(side_effect=self.get_string_side_effect)
        self.setString = StrictMock()
        self.default_return = "1"
        self.return_pairs = {}

    def get_string_side_effect(self, config_var):
        if config_var in self.return_pairs:
            return self.return_pairs[config_var]
        return self.default_return


@start_qapplication
class PlotsSettingsTest(unittest.TestCase):
    CONFIG_SERVICE_CLASSPATH = "workbench.widgets.settings.plots.presenter.ConfigService"

    def assert_connected_once(self, owner, signal):
        self.assertEqual(1, owner.receivers(signal))

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_load_current_setting_values(self, mock_ConfigService):
        # load current setting is called automatically in the constructor
        PlotSettings(None)

        mock_ConfigService.getString.assert_has_calls(
            [
                call(PlotProperties.NORMALIZATION.value),
                call(PlotProperties.SHOW_TITLE.value),
                call(PlotProperties.SHOW_LEGEND.value),
                call(PlotProperties.PLOT_FONT.value),
                call(PlotProperties.X_AXES_SCALE.value),
                call(PlotProperties.Y_AXES_SCALE.value),
                call(PlotProperties.AXES_LINE_WIDTH.value),
                call(PlotProperties.X_MIN.value),
                call(PlotProperties.X_MAX.value),
                call(PlotProperties.Y_MIN.value),
                call(PlotProperties.Y_MAX.value),
                call(PlotProperties.SHOW_TICKS_LEFT.value),
                call(PlotProperties.SHOW_TICKS_BOTTOM.value),
                call(PlotProperties.SHOW_TICKS_RIGHT.value),
                call(PlotProperties.SHOW_TICKS_TOP.value),
                call(PlotProperties.SHOW_LABELS_LEFT.value),
                call(PlotProperties.SHOW_LABELS_BOTTOM.value),
                call(PlotProperties.SHOW_LABELS_RIGHT.value),
                call(PlotProperties.SHOW_LABELS_TOP.value),
                call(PlotProperties.MAJOR_TICKS_LENGTH.value),
                call(PlotProperties.MAJOR_TICKS_WIDTH.value),
                call(PlotProperties.MAJOR_TICKS_DIRECTION.value),
                call(PlotProperties.MINOR_TICKS_LENGTH.value),
                call(PlotProperties.MINOR_TICKS_WIDTH.value),
                call(PlotProperties.MINOR_TICKS_DIRECTION.value),
                call(PlotProperties.ENABLE_GRID.value),
                call(PlotProperties.SHOW_MINOR_TICKS.value),
                call(PlotProperties.SHOW_MINOR_GRIDLINES.value),
                call(PlotProperties.LINE_STYLE.value),
                call(PlotProperties.DRAW_STYLE.value),
                call(PlotProperties.LINE_WIDTH.value),
                call(PlotProperties.MARKER_STYLE.value),
                call(PlotProperties.MARKER_SIZE.value),
                call(PlotProperties.ERROR_WIDTH.value),
                call(PlotProperties.CAPSIZE.value),
                call(PlotProperties.CAP_THICKNESS.value),
                call(PlotProperties.ERROR_EVERY.value),
                call(PlotProperties.LEGEND_LOCATION.value),
                call(PlotProperties.LEGEND_FONT_SIZE.value),
                call(PlotProperties.COLORMAP.value),
            ]
        )

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_normalization_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_normalization_changed(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.NORMALIZATION.value, "On")

        mock_ConfigService.setString.reset_mock()

        presenter.action_normalization_changed(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.NORMALIZATION.value, "Off")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_show_title_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_show_title_changed(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_TITLE.value, "On")

        mock_ConfigService.setString.reset_mock()

        presenter.action_show_title_changed(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_TITLE.value, "Off")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_default_x_axes_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_default_x_axes_changed("Linear")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.X_AXES_SCALE.value, "Linear")

        mock_ConfigService.setString.reset_mock()

        presenter.action_default_x_axes_changed("Log")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.X_AXES_SCALE.value, "Log")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_default_y_axes_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_default_y_axes_changed("Linear")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.Y_AXES_SCALE.value, "Linear")

        mock_ConfigService.setString.reset_mock()

        presenter.action_default_y_axes_changed("Log")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.Y_AXES_SCALE.value, "Log")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_axes_line_width_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_axes_line_width_changed(2)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.AXES_LINE_WIDTH.value, "2")

        mock_ConfigService.setString.reset_mock()

        presenter.action_axes_line_width_changed(3.5)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.AXES_LINE_WIDTH.value, "3.5")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_x_min_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_x_min_changed(3.2)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.X_MIN.value, "3.2")

        mock_ConfigService.setString.reset_mock()

        presenter.action_x_min_changed(1.5)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.X_MIN.value, "1.5")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_x_max_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_x_max_changed(3.2)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.X_MAX.value, "3.2")

        mock_ConfigService.setString.reset_mock()

        presenter.action_x_max_changed(1.5)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.X_MAX.value, "1.5")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_y_min_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_y_min_changed(3.2)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.Y_MIN.value, "3.2")

        mock_ConfigService.setString.reset_mock()

        presenter.action_y_min_changed(1.5)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.Y_MIN.value, "1.5")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_y_max_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_y_max_changed(3.2)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.Y_MAX.value, "3.2")

        mock_ConfigService.setString.reset_mock()

        presenter.action_y_max_changed(1.5)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.Y_MAX.value, "1.5")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_x_min_box_and_check_box_disabled_if_no_value(self, mock_ConfigService):
        mock_ConfigService.return_pairs = {PlotProperties.X_MIN.value: ""}

        presenter = PlotSettings(None)

        self.assertFalse(presenter.view.x_min.isEnabled())
        self.assertFalse(presenter.view.x_min_box.isChecked())

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_x_max_box_and_check_box_disabled_if_no_value(self, mock_ConfigService):
        mock_ConfigService.return_pairs = {PlotProperties.X_MAX.value: ""}

        presenter = PlotSettings(None)

        self.assertFalse(presenter.view.x_max.isEnabled())
        self.assertFalse(presenter.view.x_max_box.isChecked())

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_y_min_box_and_check_box_disabled_if_no_value(self, mock_ConfigService):
        mock_ConfigService.return_pairs = {PlotProperties.Y_MIN.value: ""}

        presenter = PlotSettings(None)

        self.assertFalse(presenter.view.y_min.isEnabled())
        self.assertFalse(presenter.view.y_min_box.isChecked())

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_y_max_box_and_check_box_disabled_if_no_value(self, mock_ConfigService):
        mock_ConfigService.return_pairs = {PlotProperties.Y_MAX.value: ""}

        presenter = PlotSettings(None)

        self.assertFalse(presenter.view.y_max.isEnabled())
        self.assertFalse(presenter.view.y_max_box.isChecked())

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_x_min_box_and_check_box_enabled_if_value(self, mock_ConfigService):
        mock_ConfigService.return_pairs = {PlotProperties.X_MIN.value: "50"}

        presenter = PlotSettings(None)

        self.assertTrue(presenter.view.x_min.isEnabled())
        self.assertTrue(presenter.view.x_min_box.isChecked())

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_x_max_box_and_check_box_enabled_if_value(self, mock_ConfigService):
        mock_ConfigService.return_pairs = {PlotProperties.X_MAX.value: "50"}

        presenter = PlotSettings(None)

        self.assertTrue(presenter.view.x_max.isEnabled())
        self.assertTrue(presenter.view.x_max_box.isChecked())

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_y_min_box_and_check_box_enabled_if_value(self, mock_ConfigService):
        mock_ConfigService.return_pairs = {PlotProperties.Y_MIN.value: "50"}

        presenter = PlotSettings(None)

        self.assertTrue(presenter.view.y_min.isEnabled())
        self.assertTrue(presenter.view.y_min_box.isChecked())

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_y_max_box_and_check_box_enabled_if_value(self, mock_ConfigService):
        mock_ConfigService.return_pairs = {PlotProperties.Y_MAX.value: "50"}

        presenter = PlotSettings(None)

        self.assertTrue(presenter.view.y_max.isEnabled())
        self.assertTrue(presenter.view.y_max_box.isChecked())

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_x_min_checkbox_clears_property_when_disabled(self, mock_ConfigService):
        mock_ConfigService.return_pairs = {PlotProperties.X_MIN.value: "50"}

        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_x_min_box_changed(False)

        mock_ConfigService.setString.assert_called_once_with(PlotProperties.X_MIN.value, "")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_x_max_checkbox_clears_property_when_disabled(self, mock_ConfigService):
        mock_ConfigService.return_pairs = {PlotProperties.X_MAX.value: "50"}

        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_x_max_box_changed(False)

        mock_ConfigService.setString.assert_called_once_with(PlotProperties.X_MAX.value, "")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_y_min_checkbox_clears_property_when_disabled(self, mock_ConfigService):
        mock_ConfigService.return_pairs = {PlotProperties.Y_MIN.value: "50"}

        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_y_min_box_changed(False)

        mock_ConfigService.setString.assert_called_once_with(PlotProperties.Y_MIN.value, "")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_y_max_checkbox_clears_property_when_disabled(self, mock_ConfigService):
        mock_ConfigService.return_pairs = {PlotProperties.Y_MAX.value: "50"}

        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_y_max_box_changed(False)

        mock_ConfigService.setString.assert_called_once_with(PlotProperties.Y_MAX.value, "")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_enable_grid(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_enable_grid_changed(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.ENABLE_GRID.value, "On")

        mock_ConfigService.setString.reset_mock()

        presenter.action_enable_grid_changed(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.ENABLE_GRID.value, "Off")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_show_ticks_left(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_show_ticks_left_changed(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_TICKS_LEFT.value, "On")

        mock_ConfigService.setString.reset_mock()

        presenter.action_show_ticks_left_changed(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_TICKS_LEFT.value, "Off")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_show_ticks_bottom(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_show_ticks_bottom_changed(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_TICKS_BOTTOM.value, "On")

        mock_ConfigService.setString.reset_mock()

        presenter.action_show_ticks_bottom_changed(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_TICKS_BOTTOM.value, "Off")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_show_ticks_right(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_show_ticks_right_changed(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_TICKS_RIGHT.value, "On")

        mock_ConfigService.setString.reset_mock()

        presenter.action_show_ticks_right_changed(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_TICKS_RIGHT.value, "Off")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_show_ticks_top(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_show_ticks_top_changed(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_TICKS_TOP.value, "On")

        mock_ConfigService.setString.reset_mock()

        presenter.action_show_ticks_top_changed(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_TICKS_TOP.value, "Off")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_show_labels_left(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_show_labels_left_changed(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_LABELS_LEFT.value, "On")

        mock_ConfigService.setString.reset_mock()

        presenter.action_show_labels_left_changed(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_LABELS_LEFT.value, "Off")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_show_labels_bottom(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_show_labels_bottom_changed(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_LABELS_BOTTOM.value, "On")

        mock_ConfigService.setString.reset_mock()

        presenter.action_show_labels_bottom_changed(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_LABELS_BOTTOM.value, "Off")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_show_labels_right(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_show_labels_right_changed(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_LABELS_RIGHT.value, "On")

        mock_ConfigService.setString.reset_mock()

        presenter.action_show_labels_right_changed(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_LABELS_RIGHT.value, "Off")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_show_labels_top(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_show_labels_top_changed(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_LABELS_TOP.value, "On")

        mock_ConfigService.setString.reset_mock()

        presenter.action_show_labels_top_changed(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_LABELS_TOP.value, "Off")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_line_style_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_line_style_changed("dashed")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.LINE_STYLE.value, "dashed")

        mock_ConfigService.setString.reset_mock()

        presenter.action_line_style_changed("dotted")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.LINE_STYLE.value, "dotted")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_line_width_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_line_width_changed(2)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.LINE_WIDTH.value, "2")

        mock_ConfigService.setString.reset_mock()

        presenter.action_line_width_changed(3.5)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.LINE_WIDTH.value, "3.5")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_marker_style_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_marker_style_changed("circle")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.MARKER_STYLE.value, "circle")

        mock_ConfigService.setString.reset_mock()

        presenter.action_marker_style_changed("octagon")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.MARKER_STYLE.value, "octagon")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_marker_size_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_marker_size_changed("8.0")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.MARKER_SIZE.value, "8.0")

        mock_ConfigService.setString.reset_mock()

        presenter.action_marker_size_changed("5.0")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.MARKER_SIZE.value, "5.0")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_error_width_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_error_width_changed(2)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.ERROR_WIDTH.value, "2")

        mock_ConfigService.setString.reset_mock()

        presenter.action_error_width_changed(1.5)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.ERROR_WIDTH.value, "1.5")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_capsize_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_capsize_changed(2)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.CAPSIZE.value, "2")

        mock_ConfigService.setString.reset_mock()

        presenter.action_capsize_changed(1.5)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.CAPSIZE.value, "1.5")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_cap_thickness_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_cap_thickness_changed(2)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.CAP_THICKNESS.value, "2")

        mock_ConfigService.setString.reset_mock()

        presenter.action_cap_thickness_changed(1.5)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.CAP_THICKNESS.value, "1.5")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_error_every_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_error_every_changed(2)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.ERROR_EVERY.value, "2")

        mock_ConfigService.setString.reset_mock()

        presenter.action_error_every_changed(5)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.ERROR_EVERY.value, "5")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_show_minor_ticks_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_show_minor_ticks_changed(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_MINOR_TICKS.value, "On")

        mock_ConfigService.setString.reset_mock()

        presenter.action_show_minor_ticks_changed(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_MINOR_TICKS.value, "Off")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_show_minor_gridlines_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_show_minor_gridlines_changed(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_MINOR_GRIDLINES.value, "On")

        mock_ConfigService.setString.reset_mock()

        presenter.action_show_minor_gridlines_changed(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_MINOR_GRIDLINES.value, "Off")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_show_legend_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_show_legend_changed(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_LEGEND.value, "On")

        mock_ConfigService.setString.reset_mock()

        presenter.action_show_legend_changed(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.SHOW_LEGEND.value, "Off")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_legend_every_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_legend_location_changed("best")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.LEGEND_LOCATION.value, "best")

        mock_ConfigService.setString.reset_mock()

        presenter.action_legend_location_changed("upper left")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.LEGEND_LOCATION.value, "upper left")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_legend_size_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_legend_size_changed(10)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.LEGEND_FONT_SIZE.value, "10")

        mock_ConfigService.setString.reset_mock()

        presenter.action_legend_size_changed(8)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.LEGEND_FONT_SIZE.value, "8")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_default_colormap_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        presenter.view.default_colormap_combo_box.setCurrentIndex(4)
        colormap = presenter.view.default_colormap_combo_box.currentText()

        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_default_colormap_changed()
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.COLORMAP.value, colormap)

        presenter.view.default_colormap_combo_box.setCurrentIndex(5)
        colormap = presenter.view.default_colormap_combo_box.currentText()
        presenter.view.reverse_colormap_check_box.setChecked(True)

        mock_ConfigService.setString.reset_mock()

        presenter.action_default_colormap_changed()
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.COLORMAP.value, colormap + "_r")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_font_combo_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_font_combo_changed("Helvetica")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.PLOT_FONT.value, "Helvetica")

        mock_ConfigService.setString.reset_mock()

        presenter.action_font_combo_changed("Something that is not a font")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.PLOT_FONT.value, "Something that is not a font")


if __name__ == "__main__":
    unittest.main()
