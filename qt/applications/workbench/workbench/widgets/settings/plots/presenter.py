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
from mantid.plots.utility import get_colormap_names
from mantidqt.widgets.plotconfigdialog.curvestabwidget.markertabwidget.view import MARKER_STYLES
from workbench.widgets.settings.plots.view import PlotsSettingsView
from workbench.widgets.settings.plots.model import PlotsSettingsModel
from workbench.plotting.style import VALID_LINE_STYLE, VALID_DRAW_STYLE

from qtpy.QtCore import Qt

from enum import Enum
import sys


class PlotProperties(Enum):
    CAPSIZE = "plots.errorbar.Capsize"
    CAP_THICKNESS = "plots.errorbar.CapThickness"
    X_AXES_SCALE = "plots.xAxesScale"
    Y_AXES_SCALE = "plots.yAxesScale"
    X_MIN = "plots.x_min"
    X_MAX = "plots.x_max"
    Y_MIN = "plots.y_min"
    Y_MAX = "plots.y_max"
    AXES_LINE_WIDTH = "plots.axesLineWidth"
    SHOW_TICKS_LEFT = "plots.showTicksLeft"
    SHOW_TICKS_BOTTOM = "plots.showTicksBottom"
    SHOW_TICKS_RIGHT = "plots.showTicksRight"
    SHOW_TICKS_TOP = "plots.showTicksTop"
    SHOW_LABELS_LEFT = "plots.showLabelsLeft"
    SHOW_LABELS_BOTTOM = "plots.showLabelsBottom"
    SHOW_LABELS_RIGHT = "plots.showLabelsRight"
    SHOW_LABELS_TOP = "plots.showLabelsTop"
    MAJOR_TICKS_LENGTH = "plots.ticks.major.length"
    MAJOR_TICKS_WIDTH = "plots.ticks.major.width"
    MAJOR_TICKS_DIRECTION = "plots.ticks.major.direction"
    MINOR_TICKS_LENGTH = "plots.ticks.minor.length"
    MINOR_TICKS_WIDTH = "plots.ticks.minor.width"
    MINOR_TICKS_DIRECTION = "plots.ticks.minor.direction"
    ERROR_EVERY = "plots.errorbar.errorEvery"
    ERROR_WIDTH = "plots.errorbar.Width"
    LINE_STYLE = "plots.line.Style"
    DRAW_STYLE = "plots.line.DrawStyle"
    LINE_WIDTH = "plots.line.Width"
    MARKER_STYLE = "plots.marker.Style"
    MARKER_SIZE = "plots.marker.Size"
    NORMALIZATION = "graph1d.autodistribution"
    SHOW_TITLE = "plots.ShowTitle"
    PLOT_FONT = "plots.font"
    SHOW_LEGEND = "plots.ShowLegend"
    LEGEND_FONT_SIZE = "plots.legend.FontSize"
    LEGEND_LOCATION = "plots.legend.Location"
    ENABLE_GRID = "plots.enableGrid"
    SHOW_MINOR_TICKS = "plots.ShowMinorTicks"
    SHOW_MINOR_GRIDLINES = "plots.ShowMinorGridlines"
    COLORMAP = "plots.images.Colormap"
    COLORBAR_SCALE = "plots.images.ColorBarScale"


class PlotSettings(object):
    AXES_SCALE = ["Linear", "Log"]
    AXES_Y_POSITION = ["Left", "Right"]
    AXES_X_POSITION = ["Bottom", "Top"]
    TICK_DIRECTION = ["In", "Out", "InOut"]
    LEGEND_LOCATION_LIST = [
        "best",
        "upper right",
        "center right",
        "lower right",
        "lower center",
        "lower left",
        "center left",
        "upper left",
        "upper center",
    ]

    def __init__(self, parent, view=None, model=None):
        self.view = view if view else PlotsSettingsView(parent, self)
        self.model = model if model else PlotsSettingsModel()
        self.parent = parent
        self.add_list_items()
        self.load_general_setting_values()
        self.setup_axes_group()
        self.setup_ticks_group()
        self.setup_line_group()
        self.setup_marker_group()
        self.setup_error_bar_group()
        self.setup_legend_group()
        self.setup_images_group()
        self.setup_signals()

    def add_list_items(self):
        self.view.x_axes_scale.addItems(self.AXES_SCALE)
        self.view.y_axes_scale.addItems(self.AXES_SCALE)
        self.view.major_ticks_direction.addItems(self.TICK_DIRECTION)
        self.view.minor_ticks_direction.addItems(self.TICK_DIRECTION)
        self.view.line_style.addItems(VALID_LINE_STYLE)
        self.view.draw_style.addItems(VALID_DRAW_STYLE)
        self.view.marker_style.addItems(MARKER_STYLES.keys())
        self.view.default_colormap_combo_box.addItems(get_colormap_names())
        self.view.colorbar_scale.addItems(self.AXES_SCALE)

    def load_general_setting_values(self):
        normalize_to_bin_width = "on" == ConfigService.getString(PlotProperties.NORMALIZATION.value).lower()
        show_title = "on" == ConfigService.getString(PlotProperties.SHOW_TITLE.value).lower()
        show_legend = "on" == ConfigService.getString(PlotProperties.SHOW_LEGEND.value).lower()

        self.view.normalize_to_bin_width.setChecked(normalize_to_bin_width)
        self.view.show_title.setChecked(show_title)
        self.view.show_legend.setChecked(show_legend)
        self.populate_font_combo_box()

    def setup_axes_group(self):
        x_axes_scale = ConfigService.getString(PlotProperties.X_AXES_SCALE.value)
        y_axes_scale = ConfigService.getString(PlotProperties.Y_AXES_SCALE.value)
        axes_line_width = float(ConfigService.getString(PlotProperties.AXES_LINE_WIDTH.value))
        x_min_str = ConfigService.getString(PlotProperties.X_MIN.value)
        x_max_str = ConfigService.getString(PlotProperties.X_MAX.value)
        y_min_str = ConfigService.getString(PlotProperties.Y_MIN.value)
        y_max_str = ConfigService.getString(PlotProperties.Y_MAX.value)

        if x_axes_scale in self.AXES_SCALE:
            self.view.x_axes_scale.setCurrentIndex(self.view.x_axes_scale.findText(x_axes_scale))
        else:
            self.view.x_axes_scale.setCurrentIndex(0)

        if y_axes_scale in self.AXES_SCALE:
            self.view.y_axes_scale.setCurrentIndex(self.view.y_axes_scale.findText(y_axes_scale))
        else:
            self.view.y_axes_scale.setCurrentIndex(0)

        self.view.axes_line_width.setValue(axes_line_width)

        float_max = sys.float_info.max
        self.view.x_min.setRange(-float_max, float_max)
        if x_min_str:
            self.view.x_min.setEnabled(True)
            self.view.x_min.setValue(float(x_min_str))
            self.view.x_min_box.setChecked(True)

        self.view.x_max.setRange(-float_max, float_max)
        if x_max_str:
            self.view.x_max.setEnabled(True)
            self.view.x_max.setValue(float(x_max_str))
            self.view.x_max_box.setChecked(True)

        self.view.y_min.setRange(-float_max, float_max)
        if y_min_str:
            self.view.y_min.setEnabled(True)
            self.view.y_min.setValue(float(y_min_str))
            self.view.y_min_box.setChecked(True)

        self.view.y_max.setRange(-float_max, float_max)
        if y_max_str:
            self.view.y_max.setEnabled(True)
            self.view.y_max.setValue(float(y_max_str))
            self.view.y_max_box.setChecked(True)

    def setup_ticks_group(self):
        show_ticks_left = "on" == ConfigService.getString(PlotProperties.SHOW_TICKS_LEFT.value).lower()
        show_ticks_bottom = "on" == ConfigService.getString(PlotProperties.SHOW_TICKS_BOTTOM.value).lower()
        show_ticks_right = "on" == ConfigService.getString(PlotProperties.SHOW_TICKS_RIGHT.value).lower()
        show_ticks_top = "on" == ConfigService.getString(PlotProperties.SHOW_TICKS_TOP.value).lower()
        show_labels_left = "on" == ConfigService.getString(PlotProperties.SHOW_LABELS_LEFT.value).lower()
        show_labels_bottom = "on" == ConfigService.getString(PlotProperties.SHOW_LABELS_BOTTOM.value).lower()
        show_labels_right = "on" == ConfigService.getString(PlotProperties.SHOW_LABELS_RIGHT.value).lower()
        show_labels_top = "on" == ConfigService.getString(PlotProperties.SHOW_LABELS_TOP.value).lower()
        major_ticks_length = int(ConfigService.getString(PlotProperties.MAJOR_TICKS_LENGTH.value))
        major_ticks_width = int(ConfigService.getString(PlotProperties.MAJOR_TICKS_WIDTH.value))
        major_ticks_direction = ConfigService.getString(PlotProperties.MAJOR_TICKS_DIRECTION.value)
        minor_ticks_length = int(ConfigService.getString(PlotProperties.MINOR_TICKS_LENGTH.value))
        minor_ticks_width = int(ConfigService.getString(PlotProperties.MINOR_TICKS_WIDTH.value))
        minor_ticks_direction = ConfigService.getString(PlotProperties.MINOR_TICKS_DIRECTION.value)

        enable_grid = "on" == ConfigService.getString(PlotProperties.ENABLE_GRID.value).lower()
        show_minor_ticks = "on" == ConfigService.getString(PlotProperties.SHOW_MINOR_TICKS.value).lower()
        show_minor_gridlines = "on" == ConfigService.getString(PlotProperties.SHOW_MINOR_GRIDLINES.value).lower()

        self.view.enable_grid.setChecked(enable_grid)
        self.view.show_minor_ticks.setChecked(show_minor_ticks)
        self.view.show_minor_gridlines.setEnabled(show_minor_ticks)
        self.view.show_minor_gridlines.setChecked(show_minor_gridlines)
        self.view.show_ticks_left.setChecked(show_ticks_left)
        self.view.show_ticks_bottom.setChecked(show_ticks_bottom)
        self.view.show_ticks_right.setChecked(show_ticks_right)
        self.view.show_ticks_top.setChecked(show_ticks_top)
        self.view.show_labels_left.setChecked(show_labels_left)
        self.view.show_labels_bottom.setChecked(show_labels_bottom)
        self.view.show_labels_right.setChecked(show_labels_right)
        self.view.show_labels_top.setChecked(show_labels_top)
        self.view.major_ticks_length.setValue(major_ticks_length)
        self.view.major_ticks_width.setValue(major_ticks_width)
        self.view.minor_ticks_length.setValue(minor_ticks_length)
        self.view.minor_ticks_width.setValue(minor_ticks_width)

        if major_ticks_direction in self.TICK_DIRECTION:
            self.view.major_ticks_direction.setCurrentIndex(self.view.major_ticks_direction.findText(major_ticks_direction))
        else:
            self.view.major_ticks_direction.setCurrentIndex(0)

        if minor_ticks_direction in self.TICK_DIRECTION:
            self.view.minor_ticks_direction.setCurrentIndex(self.view.minor_ticks_direction.findText(minor_ticks_direction))
        else:
            self.view.minor_ticks_direction.setCurrentIndex(0)

    def setup_line_group(self):
        line_style = ConfigService.getString(PlotProperties.LINE_STYLE.value)
        self._setup_style_combo_boxes(line_style, self.view.line_style, VALID_LINE_STYLE)

        draw_style = ConfigService.getString(PlotProperties.DRAW_STYLE.value)
        self._setup_style_combo_boxes(draw_style, self.view.draw_style, VALID_DRAW_STYLE)

        line_width = float(ConfigService.getString(PlotProperties.LINE_WIDTH.value))
        self.view.line_width.setValue(line_width)

    def setup_marker_group(self):
        marker_style = ConfigService.getString(PlotProperties.MARKER_STYLE.value)
        if marker_style in MARKER_STYLES.keys():
            self.view.marker_style.setCurrentIndex(self.view.marker_style.findText(marker_style))
        else:
            self.view.marker_style.setCurrentIndex(0)

        marker_size = float(ConfigService.getString(PlotProperties.MARKER_SIZE.value))
        self.view.marker_size.setValue(marker_size)

    def setup_error_bar_group(self):
        error_width = float(ConfigService.getString(PlotProperties.ERROR_WIDTH.value))
        capsize = float(ConfigService.getString(PlotProperties.CAPSIZE.value))
        cap_thickness = float(ConfigService.getString(PlotProperties.CAP_THICKNESS.value))
        error_every = int(ConfigService.getString(PlotProperties.ERROR_EVERY.value))

        self.view.error_width.setValue(error_width)
        self.view.capsize.setValue(capsize)
        self.view.cap_thickness.setValue(cap_thickness)
        self.view.error_every.setValue(error_every)

    def setup_legend_group(self):
        legend_location = ConfigService.getString(PlotProperties.LEGEND_LOCATION.value)
        self.view.legend_location.addItems(self.LEGEND_LOCATION_LIST)
        if legend_location in self.LEGEND_LOCATION_LIST:
            self.view.legend_location.setCurrentIndex(self.view.legend_location.findText(legend_location))
        legend_font_size = float(ConfigService.getString(PlotProperties.LEGEND_FONT_SIZE.value))
        self.view.legend_font_size.setValue(legend_font_size)

    def setup_images_group(self):
        colormap = ConfigService.getString(PlotProperties.COLORMAP.value)
        if self.view.default_colormap_combo_box.findText(colormap) != -1:
            self.view.default_colormap_combo_box.setCurrentIndex(self.view.default_colormap_combo_box.findText(colormap))
            self.view.reverse_colormap_check_box.setChecked(False)
        elif colormap.endswith("_r") and self.view.default_colormap_combo_box.findText(colormap[:-2]):
            self.view.default_colormap_combo_box.setCurrentIndex(self.view.default_colormap_combo_box.findText(colormap[:-2]))
            self.view.reverse_colormap_check_box.setChecked(True)
        colorbar_scale = ConfigService.getString(PlotProperties.COLORBAR_SCALE.value)
        if colorbar_scale in self.AXES_SCALE:
            self.view.colorbar_scale.setCurrentIndex(self.view.colorbar_scale.findText(colorbar_scale))
        else:
            self.view.colorbar_scale.setCurrentIndex(0)

    @staticmethod
    def _setup_style_combo_boxes(current_style, style_combo, combo_items):
        if current_style in combo_items:
            style_combo.setCurrentIndex(style_combo.findText(current_style))
        else:
            style_combo.setCurrentIndex(0)

    def setup_signals(self):
        self.view.normalize_to_bin_width.stateChanged.connect(self.action_normalization_changed)
        self.view.show_title.stateChanged.connect(self.action_show_title_changed)
        self.view.show_legend.stateChanged.connect(self.action_show_legend_changed)

        # Axes
        self.view.x_axes_scale.currentTextChanged.connect(self.action_default_x_axes_changed)
        self.view.y_axes_scale.currentTextChanged.connect(self.action_default_y_axes_changed)
        self.view.axes_line_width.valueChanged.connect(self.action_axes_line_width_changed)

        # Lines
        self.view.line_style.currentTextChanged.connect(self.action_line_style_changed)
        self.view.draw_style.currentTextChanged.connect(self.action_draw_style_changed)
        self.view.line_width.valueChanged.connect(self.action_line_width_changed)
        self.view.x_min.valueChanged.connect(self.action_x_min_changed)
        self.view.x_max.valueChanged.connect(self.action_x_max_changed)
        self.view.y_min.valueChanged.connect(self.action_y_min_changed)
        self.view.y_max.valueChanged.connect(self.action_y_max_changed)
        self.view.x_min_box.stateChanged.connect(self.action_x_min_box_changed)
        self.view.x_max_box.stateChanged.connect(self.action_x_max_box_changed)
        self.view.y_min_box.stateChanged.connect(self.action_y_min_box_changed)
        self.view.y_max_box.stateChanged.connect(self.action_y_max_box_changed)

        # Ticks
        self.view.enable_grid.stateChanged.connect(self.action_enable_grid_changed)
        self.view.show_minor_ticks.stateChanged.connect(self.action_show_minor_ticks_changed)
        self.view.show_minor_gridlines.stateChanged.connect(self.action_show_minor_gridlines_changed)
        self.view.show_ticks_left.stateChanged.connect(self.action_show_ticks_left_changed)
        self.view.show_ticks_bottom.stateChanged.connect(self.action_show_ticks_bottom_changed)
        self.view.show_ticks_right.stateChanged.connect(self.action_show_ticks_right_changed)
        self.view.show_ticks_top.stateChanged.connect(self.action_show_ticks_top_changed)
        self.view.show_labels_left.stateChanged.connect(self.action_show_labels_left_changed)
        self.view.show_labels_bottom.stateChanged.connect(self.action_show_labels_bottom_changed)
        self.view.show_labels_right.stateChanged.connect(self.action_show_labels_right_changed)
        self.view.show_labels_top.stateChanged.connect(self.action_show_labels_top_changed)
        self.view.major_ticks_length.valueChanged.connect(self.action_major_ticks_length_changed)
        self.view.major_ticks_width.valueChanged.connect(self.action_major_ticks_width_changed)
        self.view.major_ticks_direction.currentTextChanged.connect(self.action_major_ticks_direction_changed)
        self.view.minor_ticks_length.valueChanged.connect(self.action_minor_ticks_length_changed)
        self.view.minor_ticks_width.valueChanged.connect(self.action_minor_ticks_width_changed)
        self.view.minor_ticks_direction.currentTextChanged.connect(self.action_minor_ticks_direction_changed)

        # Markers
        self.view.marker_style.currentTextChanged.connect(self.action_marker_style_changed)
        self.view.marker_size.valueChanged.connect(self.action_marker_size_changed)

        # Error bars
        self.view.error_width.valueChanged.connect(self.action_error_width_changed)
        self.view.capsize.valueChanged.connect(self.action_capsize_changed)
        self.view.cap_thickness.valueChanged.connect(self.action_cap_thickness_changed)
        self.view.error_every.valueChanged.connect(self.action_error_every_changed)

        # Legend
        self.view.legend_location.currentTextChanged.connect(self.action_legend_location_changed)
        self.view.legend_font_size.valueChanged.connect(self.action_legend_size_changed)

        # Images
        self.view.default_colormap_combo_box.currentTextChanged.connect(self.action_default_colormap_changed)
        self.view.reverse_colormap_check_box.stateChanged.connect(self.action_default_colormap_changed)
        self.view.plot_font.currentTextChanged.connect(self.action_font_combo_changed)
        self.view.colorbar_scale.currentTextChanged.connect(self.action_colorbar_scale_changed)

    def action_normalization_changed(self, state):
        ConfigService.setString(PlotProperties.NORMALIZATION.value, "On" if state == Qt.Checked else "Off")

    def action_show_title_changed(self, state):
        ConfigService.setString(PlotProperties.SHOW_TITLE.value, "On" if state == Qt.Checked else "Off")

    def action_enable_grid_changed(self, state):
        ConfigService.setString(PlotProperties.ENABLE_GRID.value, "On" if state == Qt.Checked else "Off")

    def action_show_minor_ticks_changed(self, state):
        ConfigService.setString(PlotProperties.SHOW_MINOR_TICKS.value, "On" if state == Qt.Checked else "Off")
        self.view.show_minor_gridlines.setEnabled(state == Qt.Checked)

        if not self.view.show_minor_gridlines.isEnabled():
            self.view.show_minor_gridlines.setChecked(False)

    def action_show_minor_gridlines_changed(self, state):
        ConfigService.setString(PlotProperties.SHOW_MINOR_GRIDLINES.value, "On" if state == Qt.Checked else "Off")

    def action_font_combo_changed(self, font_name):
        ConfigService.setString(PlotProperties.PLOT_FONT.value, font_name)

    def action_show_legend_changed(self, state):
        ConfigService.setString(PlotProperties.SHOW_LEGEND.value, "On" if state == Qt.Checked else "Off")

    def action_default_x_axes_changed(self, axes_scale):
        ConfigService.setString(PlotProperties.X_AXES_SCALE.value, axes_scale)

    def action_default_y_axes_changed(self, axes_scale):
        ConfigService.setString(PlotProperties.Y_AXES_SCALE.value, axes_scale)

    def action_axes_line_width_changed(self, width):
        ConfigService.setString(PlotProperties.AXES_LINE_WIDTH.value, str(width))

    def action_show_ticks_left_changed(self, state):
        ConfigService.setString(PlotProperties.SHOW_TICKS_LEFT.value, "On" if state == Qt.Checked else "Off")

    def action_show_ticks_bottom_changed(self, state):
        ConfigService.setString(PlotProperties.SHOW_TICKS_BOTTOM.value, "On" if state == Qt.Checked else "Off")

    def action_show_ticks_right_changed(self, state):
        ConfigService.setString(PlotProperties.SHOW_TICKS_RIGHT.value, "On" if state == Qt.Checked else "Off")

    def action_show_ticks_top_changed(self, state):
        ConfigService.setString(PlotProperties.SHOW_TICKS_TOP.value, "On" if state == Qt.Checked else "Off")

    def action_show_labels_left_changed(self, state):
        ConfigService.setString(PlotProperties.SHOW_LABELS_LEFT.value, "On" if state == Qt.Checked else "Off")

    def action_show_labels_bottom_changed(self, state):
        ConfigService.setString(PlotProperties.SHOW_LABELS_BOTTOM.value, "On" if state == Qt.Checked else "Off")

    def action_show_labels_right_changed(self, state):
        ConfigService.setString(PlotProperties.SHOW_LABELS_RIGHT.value, "On" if state == Qt.Checked else "Off")

    def action_show_labels_top_changed(self, state):
        ConfigService.setString(PlotProperties.SHOW_LABELS_TOP.value, "On" if state == Qt.Checked else "Off")

    def action_major_ticks_length_changed(self, value):
        ConfigService.setString(PlotProperties.MAJOR_TICKS_LENGTH.value, str(value))

    def action_major_ticks_width_changed(self, value):
        ConfigService.setString(PlotProperties.MAJOR_TICKS_WIDTH.value, str(value))

    def action_major_ticks_direction_changed(self, direction):
        ConfigService.setString(PlotProperties.MAJOR_TICKS_DIRECTION.value, direction)

    def action_minor_ticks_length_changed(self, value):
        ConfigService.setString(PlotProperties.MINOR_TICKS_LENGTH.value, str(value))

    def action_minor_ticks_width_changed(self, value):
        ConfigService.setString(PlotProperties.MINOR_TICKS_WIDTH.value, str(value))

    def action_minor_ticks_direction_changed(self, direction):
        ConfigService.setString(PlotProperties.MINOR_TICKS_DIRECTION.value, direction)

    def action_line_style_changed(self, style):
        ConfigService.setString(PlotProperties.LINE_STYLE.value, style)

    def action_draw_style_changed(self, style):
        ConfigService.setString(PlotProperties.DRAW_STYLE.value, style)

    def action_line_width_changed(self, value):
        ConfigService.setString(PlotProperties.LINE_WIDTH.value, str(value))

    def action_x_min_changed(self, value):
        ConfigService.setString(PlotProperties.X_MIN.value, str(value))

    def action_x_max_changed(self, value):
        ConfigService.setString(PlotProperties.X_MAX.value, str(value))

    def action_y_min_changed(self, value):
        ConfigService.setString(PlotProperties.Y_MIN.value, str(value))

    def action_y_max_changed(self, value):
        ConfigService.setString(PlotProperties.Y_MAX.value, str(value))

    def action_x_min_box_changed(self, state):
        self.view.x_min.setEnabled(state)
        if state:
            self.action_x_min_changed(self.view.x_min.value())
        else:
            self.action_x_min_changed("")

    def action_x_max_box_changed(self, state):
        self.view.x_max.setEnabled(state)
        if state:
            self.action_x_max_changed(self.view.x_max.value())
        else:
            self.action_x_max_changed("")

    def action_y_min_box_changed(self, state):
        self.view.y_min.setEnabled(state)
        if state:
            self.action_y_min_changed(self.view.y_min.value())
        else:
            self.action_y_min_changed("")

    def action_y_max_box_changed(self, state):
        self.view.y_max.setEnabled(state)
        if state:
            self.action_y_max_changed(self.view.y_max.value())
        else:
            self.action_y_max_changed("")

    def action_marker_style_changed(self, style):
        ConfigService.setString(PlotProperties.MARKER_STYLE.value, style)

    def action_marker_size_changed(self, value):
        ConfigService.setString(PlotProperties.MARKER_SIZE.value, str(value))

    def action_error_width_changed(self, value):
        ConfigService.setString(PlotProperties.ERROR_WIDTH.value, str(value))

    def action_capsize_changed(self, value):
        ConfigService.setString(PlotProperties.CAPSIZE.value, str(value))

    def action_cap_thickness_changed(self, value):
        ConfigService.setString(PlotProperties.CAP_THICKNESS.value, str(value))

    def action_error_every_changed(self, value):
        ConfigService.setString(PlotProperties.ERROR_EVERY.value, str(value))

    def action_legend_location_changed(self, location):
        ConfigService.setString(PlotProperties.LEGEND_LOCATION.value, str(location))

    def action_legend_size_changed(self, value):
        ConfigService.setString(PlotProperties.LEGEND_FONT_SIZE.value, str(value))

    def action_colorbar_scale_changed(self, value):
        ConfigService.setString(PlotProperties.COLORBAR_SCALE.value, value)

    def action_default_colormap_changed(self):
        colormap = self.view.default_colormap_combo_box.currentText()
        if self.view.reverse_colormap_check_box.isChecked():
            colormap += "_r"

        ConfigService.setString(PlotProperties.COLORMAP.value, colormap)

    def populate_font_combo_box(self):
        fonts = self.model.get_font_names()
        self.view.plot_font.addItems(fonts)
        current_font = ConfigService.getString(PlotProperties.PLOT_FONT.value)
        if current_font:
            self.view.plot_font.setCurrentText(current_font)
        else:
            self.view.plot_font.setCurrentText(self.model.get_current_mpl_font())

    def update_properties(self):
        self.load_general_setting_values()
        self.setup_axes_group()
        self.setup_line_group()
        self.setup_marker_group()
        self.setup_error_bar_group()
        self.setup_legend_group()
        self.setup_images_group()
