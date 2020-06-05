# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
#
#
from mantid.kernel import ConfigService
from workbench.widgets.settings.plots.view import PlotsSettingsView
from workbench.plotting.style import VALID_LINE_STYLE, VALID_DRAW_STYLE
from mantidqt.widgets.plotconfigdialog.curvestabwidget.markertabwidget.view import MARKER_STYLES

from qtpy.QtCore import Qt


class PlotSettings(object):
    CAPSIZE = "plots.errorbar.Capsize"
    CAP_THICKNESS = "plots.errorbar.CapThickness"
    X_AXES_SCALE = "plots.xAxesScale"
    Y_AXES_SCALE = "plots.yAxesScale"
    ERROR_EVERY = "plots.errorbar.errorEvery"
    ERROR_WIDTH = "plots.errorbar.Width"
    LINE_STYLE = "plots.line.Style"
    DRAW_STYLE = "plots.line.DrawStyle"
    LINE_WIDTH = "plots.line.Width"
    MARKER_STYLE = "plots.marker.Style"
    MARKER_SIZE = "plots.marker.Size"
    NORMALIZATION = "graph1d.autodistribution"
    SHOW_TITLE = "plots.ShowTitle"
    LEGEND_LOCATION = "plots.LegendLocation"
    AXES_SCALE = ['Linear', 'Log']
    LEGEND_LOCATION_LIST = ['best', 'upper right', 'center right', 'lower right', 'lower center', 'lower left',
                            'center left', 'upper left', 'upper center']
    SHOW_MINOR_TICKS = "plots.ShowMinorTicks"
    SHOW_MINOR_GRIDLINES = "plots.ShowMinorGridlines"

    def __init__(self, parent, view=None):
        self.view = view if view else PlotsSettingsView(parent, self)
        self.parent = parent
        self.load_general_setting_values()
        self.setup_axes_group()
        self.setup_line_group()
        self.setup_marker_group()
        self.setup_error_bar_group()
        self.setup_layout_group()
        self.setup_signals()

    def load_general_setting_values(self):
        normalize_to_bin_width = "on" == ConfigService.getString(self.NORMALIZATION).lower()
        show_title = "on" == ConfigService.getString(self.SHOW_TITLE).lower()
        show_minor_ticks = "on" == ConfigService.getString(self.SHOW_MINOR_TICKS).lower()
        show_minor_gridlines = "on" == ConfigService.getString(self.SHOW_MINOR_GRIDLINES).lower()

        self.view.normalize_to_bin_width.setChecked(normalize_to_bin_width)
        self.view.show_title.setChecked(show_title)
        self.view.show_minor_ticks.setChecked(show_minor_ticks)
        self.view.show_minor_gridlines.setEnabled(show_minor_ticks)
        self.view.show_minor_gridlines.setEnabled(show_minor_gridlines)

    def setup_axes_group(self):
        x_axes_scale = ConfigService.getString(self.X_AXES_SCALE)
        y_axes_scale = ConfigService.getString(self.Y_AXES_SCALE)

        self.view.x_axes_scale.addItems(self.AXES_SCALE)
        self.view.y_axes_scale.addItems(self.AXES_SCALE)
        if x_axes_scale in self.AXES_SCALE:
            self.view.x_axes_scale.setCurrentIndex(self.view.x_axes_scale.findText(x_axes_scale))
        else:
            self.view.x_axes_scale.setCurrentIndex(0)

        if y_axes_scale in self.AXES_SCALE:
            self.view.y_axes_scale.setCurrentIndex(self.view.y_axes_scale.findText(y_axes_scale))
        else:
            self.view.y_axes_scale.setCurrentIndex(0)

    def setup_line_group(self):
        line_style = ConfigService.getString(self.LINE_STYLE)
        self._setup_style_combo_boxes(line_style, self.view.line_style, VALID_LINE_STYLE)

        draw_style = ConfigService.getString(self.DRAW_STYLE)
        self._setup_style_combo_boxes(draw_style, self.view.draw_style, VALID_DRAW_STYLE)

        line_width = float(ConfigService.getString(self.LINE_WIDTH))
        self.view.line_width.setValue(line_width)

    def setup_marker_group(self):
        marker_style = ConfigService.getString(self.MARKER_STYLE)
        self.view.marker_style.addItems(MARKER_STYLES.keys())
        if marker_style in MARKER_STYLES.keys():
            self.view.marker_style.setCurrentIndex(self.view.marker_style.findText(marker_style))
        else:
            self.view.marker_style.setCurrentIndex(0)

        marker_size = float(ConfigService.getString(self.MARKER_SIZE))
        self.view.marker_size.setValue(marker_size)

    def setup_error_bar_group(self):
        error_width = float(ConfigService.getString(self.ERROR_WIDTH))
        capsize = float(ConfigService.getString(self.CAPSIZE))
        cap_thickness = float(ConfigService.getString(self.CAP_THICKNESS))
        error_every = int(ConfigService.getString(self.ERROR_EVERY))

        self.view.error_width.setValue(error_width)
        self.view.capsize.setValue(capsize)
        self.view.cap_thickness.setValue(cap_thickness)
        self.view.error_every.setValue(error_every)

    def setup_layout_group(self):
        legend_location = ConfigService.getString(self.LEGEND_LOCATION)
        self.view.legend_location.addItems(self.LEGEND_LOCATION_LIST)
        if legend_location in self.LEGEND_LOCATION_LIST:
            self.view.legend_location.setCurrentIndex(self.view.legend_location.findText(legend_location))

    @staticmethod
    def _setup_style_combo_boxes(current_style, style_combo, combo_items):
        style_combo.addItems(combo_items)
        if current_style in combo_items:
            style_combo.setCurrentIndex(style_combo.findText(current_style))
        else:
            style_combo.setCurrentIndex(0)

    def setup_signals(self):
        self.view.normalize_to_bin_width.stateChanged.connect(self.action_normalization_changed)
        self.view.show_title.stateChanged.connect(self.action_show_title_changed)
        self.view.show_minor_ticks.stateChanged.connect(self.action_show_minor_ticks_changed)
        self.view.show_minor_gridlines.stateChanged.connect(self.action_show_minor_gridlines_changed)
        self.view.x_axes_scale.currentTextChanged.connect(self.action_default_x_axes_changed)
        self.view.y_axes_scale.currentTextChanged.connect(self.action_default_y_axes_changed)
        self.view.line_style.currentTextChanged.connect(self.action_line_style_changed)
        self.view.draw_style.currentTextChanged.connect(self.action_draw_style_changed)
        self.view.line_width.valueChanged.connect(self.action_line_width_changed)
        self.view.marker_style.currentTextChanged.connect(self.action_marker_style_changed)
        self.view.marker_size.valueChanged.connect(self.action_marker_size_changed)
        self.view.error_width.valueChanged.connect(self.action_error_width_changed)
        self.view.capsize.valueChanged.connect(self.action_capsize_changed)
        self.view.cap_thickness.valueChanged.connect(self.action_cap_thickness_changed)
        self.view.error_every.valueChanged.connect(self.action_error_every_changed)
        self.view.legend_location.currentTextChanged.connect(self.action_legend_location_changed)

    def action_normalization_changed(self, state):
        ConfigService.setString(self.NORMALIZATION, "On" if state == Qt.Checked else "Off")

    def action_show_title_changed(self, state):
        ConfigService.setString(self.SHOW_TITLE, "On" if state == Qt.Checked else "Off")

    def action_show_minor_ticks_changed(self, state):
        ConfigService.setString(self.SHOW_MINOR_TICKS, "On" if state == Qt.Checked else "Off")
        self.view.show_minor_gridlines.setEnabled(state == Qt.Checked)

        if not self.view.show_minor_gridlines.isEnabled():
            self.view.show_minor_gridlines.setChecked(False)

    def action_show_minor_gridlines_changed(self, state):
        ConfigService.setString(self.SHOW_MINOR_GRIDLINES, "On" if state == Qt.Checked else "Off")

    def action_default_x_axes_changed(self, axes_scale):
        ConfigService.setString(self.X_AXES_SCALE, axes_scale)

    def action_default_y_axes_changed(self, axes_scale):
        ConfigService.setString(self.Y_AXES_SCALE, axes_scale)

    def action_line_style_changed(self, style):
        ConfigService.setString(self.LINE_STYLE, style)

    def action_draw_style_changed(self, style):
        ConfigService.setString(self.DRAW_STYLE, style)

    def action_line_width_changed(self, value):
        ConfigService.setString(self.LINE_WIDTH, str(value))

    def action_marker_style_changed(self, style):
        ConfigService.setString(self.MARKER_STYLE, style)

    def action_marker_size_changed(self, value):
        ConfigService.setString(self.MARKER_SIZE, str(value))

    def action_error_width_changed(self, value):
        ConfigService.setString(self.ERROR_WIDTH, str(value))

    def action_capsize_changed(self, value):
        ConfigService.setString(self.CAPSIZE, str(value))

    def action_cap_thickness_changed(self, value):
        ConfigService.setString(self.CAP_THICKNESS, str(value))

    def action_error_every_changed(self, value):
        ConfigService.setString(self.ERROR_EVERY, str(value))

    def action_legend_location_changed(self, location):
        ConfigService.setString(self.LEGEND_LOCATION, str(location))
