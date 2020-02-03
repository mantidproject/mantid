# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
#
#
from __future__ import absolute_import, unicode_literals

from mantid.kernel import ConfigService
from workbench.widgets.settings.plots.view import PlotsSettingsView
from workbench.plotting.style import VALID_LINE_STYLE
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
    LINE_WIDTH = "plots.line.Width"
    MARKER_STYLE = "plots.marker.Style"
    MARKER_SIZE = "plots.marker.Size"
    NORMALIZATION = "graph1d.autodistribution"
    SHOW_TITLE = "plots.ShowTitle"
    AXES_SCALE = ['Linear', 'Log']

    def __init__(self, parent, view=None):
        self.view = view if view else PlotsSettingsView(parent, self)
        self.parent = parent
        self.load_general_setting_values()
        self.setup_axes_group()
        self.setup_line_group()
        self.setup_marker_group()
        self.setup_error_bar_group()
        self.setup_signals()

    def load_general_setting_values(self):
        normalize_to_bin_width = "on" == ConfigService.getString(self.NORMALIZATION).lower()
        show_title = "on" == ConfigService.getString(self.SHOW_TITLE).lower()

        self.view.normalize_to_bin_width.setChecked(normalize_to_bin_width)
        self.view.show_title.setChecked(show_title)

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
        self.view.line_style.addItems(VALID_LINE_STYLE)
        if line_style in VALID_LINE_STYLE:
            self.view.line_style.setCurrentIndex(self.view.line_style.findText(line_style))
        else:
            self.view.line_style.setCurrentIndex(0)

        line_width = float(ConfigService.getString(self.LINE_WIDTH))
        self.view.line_width.setValue(line_width)

    def setup_marker_group(self):
        marker_style = ConfigService.getString(self.MARKER_STYLE)
        self.view.marker_style.addItems(MARKER_STYLES)
        if marker_style in MARKER_STYLES:
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

    def setup_signals(self):
        self.view.normalize_to_bin_width.stateChanged.connect(self.action_normalization_changed)
        self.view.show_title.stateChanged.connect(self.action_show_title_changed)
        self.view.x_axes_scale.currentTextChanged.connect(self.action_default_x_axes_changed)
        self.view.y_axes_scale.currentTextChanged.connect(self.action_default_y_axes_changed)
        self.view.line_style.currentTextChanged.connect(self.action_line_style_changed)
        self.view.line_width.valueChanged.connect(self.action_line_width_changed)
        self.view.marker_style.currentTextChanged.connect(self.action_marker_style_changed)
        self.view.marker_size.valueChanged.connect(self.action_marker_size_changed)
        self.view.error_width.valueChanged.connect(self.action_error_width_changed)
        self.view.capsize.valueChanged.connect(self.action_capsize_changed)
        self.view.cap_thickness.valueChanged.connect(self.action_cap_thickness_changed)
        self.view.error_every.valueChanged.connect(self.action_error_every_changed)

    def action_normalization_changed(self, state):
        ConfigService.setString(self.NORMALIZATION, "On" if state == Qt.Checked else "Off")

    def action_show_title_changed(self, state):
        ConfigService.setString(self.SHOW_TITLE, "On" if state == Qt.Checked else "Off")

    def action_default_x_axes_changed(self, axes_scale):
        ConfigService.setString(self.X_AXES_SCALE, axes_scale)

    def action_default_y_axes_changed(self, axes_scale):
        ConfigService.setString(self.Y_AXES_SCALE, axes_scale)

    def action_line_style_changed(self, style):
        ConfigService.setString(self.LINE_STYLE, style)

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
