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
        self.getString = StrictMock(return_value="1")
        self.setString = StrictMock()


@start_qapplication
class PlotsSettingsTest(unittest.TestCase):
    CONFIG_SERVICE_CLASSPATH = "workbench.widgets.settings.plots.presenter.ConfigService"

    def assert_connected_once(self, owner, signal):
        self.assertEqual(1, owner.receivers(signal))

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_load_current_setting_values(self, mock_ConfigService):
        # load current setting is called automatically in the constructor
        PlotSettings(None)

        mock_ConfigService.getString.assert_has_calls([call(PlotProperties.NORMALIZATION.value),
                                                       call(PlotProperties.SHOW_TITLE.value),
                                                       call(PlotProperties.SHOW_MINOR_TICKS.value),
                                                       call(PlotProperties.SHOW_MINOR_GRIDLINES.value),
                                                       call(PlotProperties.PLOT_FONT.value),
                                                       call(PlotProperties.X_AXES_SCALE.value),
                                                       call(PlotProperties.Y_AXES_SCALE.value),
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
                                                       call(PlotProperties.COLORMAP.value)])

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

        presenter.action_marker_style_changed('circle')
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.MARKER_STYLE.value, "circle")

        mock_ConfigService.setString.reset_mock()

        presenter.action_marker_style_changed('octagon')
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.MARKER_STYLE.value, "octagon")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_marker_size_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_marker_size_changed('8.0')
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.MARKER_SIZE.value, "8.0")

        mock_ConfigService.setString.reset_mock()

        presenter.action_marker_size_changed('5.0')
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
    def test_action_legend_every_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_legend_location_changed('best')
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.LEGEND_LOCATION.value, 'best')

        mock_ConfigService.setString.reset_mock()

        presenter.action_legend_location_changed('upper left')
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.LEGEND_LOCATION.value, 'upper left')

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_legend_size_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_legend_size_changed(10)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.LEGEND_FONT_SIZE.value, '10')

        mock_ConfigService.setString.reset_mock()

        presenter.action_legend_size_changed(8)
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.LEGEND_FONT_SIZE.value, '8')

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
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.COLORMAP.value, colormap+"_r")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_font_combo_changed(self, mock_ConfigService):
        presenter = PlotSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_font_combo_changed("Helvetica")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.PLOT_FONT.value, "Helvetica")

        mock_ConfigService.setString.reset_mock()

        presenter.action_font_combo_changed("Something that is not a font")
        mock_ConfigService.setString.assert_called_once_with(PlotProperties.PLOT_FONT.value,
                                                             "Something that is not a font")


if __name__ == "__main__":
    unittest.main()
