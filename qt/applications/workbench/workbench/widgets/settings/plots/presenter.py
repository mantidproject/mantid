# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
#
#
from mantid.plots.utility import get_colormap_names
from mantidqt.widgets.plotconfigdialog.curvestabwidget.markertabwidget.view import MARKER_STYLES
from workbench.widgets.settings.plots.view import PlotsSettingsView
from workbench.widgets.settings.view_utilities.settings_view_utilities import filter_out_mousewheel_events_from_combo_or_spin_box
from workbench.plotting.style import VALID_LINE_STYLE, VALID_DRAW_STYLE

from qtpy.QtCore import Qt

import sys


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

    def __init__(self, parent, model, view=None):
        self._view = view if view else PlotsSettingsView(parent, self)
        self._model = model
        self.parent = parent
        self.add_filters()
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

    def get_view(self):
        return self._view

    def add_filters(self):
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.plot_font)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.x_axes_scale)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.y_axes_scale)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.axes_line_width)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.x_min)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.x_max)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.y_min)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.y_max)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.major_ticks_length)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.major_ticks_width)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.major_ticks_direction)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.minor_ticks_length)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.minor_ticks_width)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.minor_ticks_direction)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.line_style)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.draw_style)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.line_width)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.marker_style)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.marker_size)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.error_width)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.capsize)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.cap_thickness)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.error_every)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.legend_font_size)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.legend_location)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.colorbar_scale)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.default_colormap_combo_box)

    def add_list_items(self):
        self._view.x_axes_scale.addItems(self.AXES_SCALE)
        self._view.y_axes_scale.addItems(self.AXES_SCALE)
        self._view.major_ticks_direction.addItems(self.TICK_DIRECTION)
        self._view.minor_ticks_direction.addItems(self.TICK_DIRECTION)
        self._view.line_style.addItems(VALID_LINE_STYLE)
        self._view.draw_style.addItems(VALID_DRAW_STYLE)
        self._view.marker_style.addItems(MARKER_STYLES.keys())
        self._view.default_colormap_combo_box.addItems(get_colormap_names())
        self._view.colorbar_scale.addItems(self.AXES_SCALE)

    def load_general_setting_values(self):
        normalize_to_bin_width = "on" == self._model.get_normalize_to_bin_width().lower()
        show_title = "on" == self._model.get_show_title().lower()
        show_legend = "on" == self._model.get_show_legend().lower()

        self._view.normalize_to_bin_width.setChecked(normalize_to_bin_width)
        self._view.show_title.setChecked(show_title)
        self._view.show_legend.setChecked(show_legend)
        self.populate_font_combo_box()

    def setup_axes_group(self):
        x_axes_scale = self._model.get_x_axes_scale()
        y_axes_scale = self._model.get_y_axes_scale()
        axes_line_width = float(self._model.get_axes_line_width())
        x_min_str = self._model.get_x_min()
        x_max_str = self._model.get_x_max()
        y_min_str = self._model.get_y_min()
        y_max_str = self._model.get_y_max()

        self._setup_style_combo_boxes(x_axes_scale, self._view.x_axes_scale, self.AXES_SCALE)
        self._setup_style_combo_boxes(y_axes_scale, self._view.y_axes_scale, self.AXES_SCALE)

        self._view.axes_line_width.setValue(axes_line_width)

        float_max = sys.float_info.max
        self._view.x_min.setRange(-float_max, float_max)
        if x_min_str:
            self._view.x_min.setEnabled(True)
            self._view.x_min.setValue(float(x_min_str))
            self._view.x_min_box.setChecked(True)

        self._view.x_max.setRange(-float_max, float_max)
        if x_max_str:
            self._view.x_max.setEnabled(True)
            self._view.x_max.setValue(float(x_max_str))
            self._view.x_max_box.setChecked(True)

        self._view.y_min.setRange(-float_max, float_max)
        if y_min_str:
            self._view.y_min.setEnabled(True)
            self._view.y_min.setValue(float(y_min_str))
            self._view.y_min_box.setChecked(True)

        self._view.y_max.setRange(-float_max, float_max)
        if y_max_str:
            self._view.y_max.setEnabled(True)
            self._view.y_max.setValue(float(y_max_str))
            self._view.y_max_box.setChecked(True)

    def setup_ticks_group(self):
        show_ticks_left = "on" == self._model.get_show_ticks_left().lower()
        show_ticks_bottom = "on" == self._model.get_show_ticks_bottom().lower()
        show_ticks_right = "on" == self._model.get_show_ticks_right().lower()
        show_ticks_top = "on" == self._model.get_show_ticks_top().lower()
        show_labels_left = "on" == self._model.get_show_labels_left().lower()
        show_labels_bottom = "on" == self._model.get_show_labels_bottom().lower()
        show_labels_right = "on" == self._model.get_show_labels_right().lower()
        show_labels_top = "on" == self._model.get_show_labels_top().lower()
        major_ticks_length = int(self._model.get_major_ticks_length())
        major_ticks_width = int(self._model.get_major_ticks_width())
        major_ticks_direction = self._model.get_major_ticks_direction()
        minor_ticks_length = int(self._model.get_minor_ticks_length())
        minor_ticks_width = int(self._model.get_minor_ticks_width())
        minor_ticks_direction = self._model.get_minor_ticks_direction()

        enable_grid = "on" == self._model.get_enable_grid().lower()
        show_minor_ticks = "on" == self._model.get_show_minor_ticks().lower()
        show_minor_gridlines = "on" == self._model.get_show_minor_gridlines().lower()

        self._view.enable_grid.setChecked(enable_grid)
        self._view.show_minor_ticks.setChecked(show_minor_ticks)
        self._view.show_minor_gridlines.setEnabled(show_minor_ticks)
        self._view.show_minor_gridlines.setChecked(show_minor_gridlines)
        self._view.show_ticks_left.setChecked(show_ticks_left)
        self._view.show_ticks_bottom.setChecked(show_ticks_bottom)
        self._view.show_ticks_right.setChecked(show_ticks_right)
        self._view.show_ticks_top.setChecked(show_ticks_top)
        self._view.show_labels_left.setChecked(show_labels_left)
        self._view.show_labels_bottom.setChecked(show_labels_bottom)
        self._view.show_labels_right.setChecked(show_labels_right)
        self._view.show_labels_top.setChecked(show_labels_top)
        self._view.major_ticks_length.setValue(major_ticks_length)
        self._view.major_ticks_width.setValue(major_ticks_width)
        self._view.minor_ticks_length.setValue(minor_ticks_length)
        self._view.minor_ticks_width.setValue(minor_ticks_width)

        self._setup_style_combo_boxes(major_ticks_direction, self._view.major_ticks_direction, self.TICK_DIRECTION)
        self._setup_style_combo_boxes(minor_ticks_direction, self._view.minor_ticks_direction, self.TICK_DIRECTION)

    def setup_line_group(self):
        line_style = self._model.get_line_style()
        self._setup_style_combo_boxes(line_style, self._view.line_style, VALID_LINE_STYLE)

        draw_style = self._model.get_draw_style()
        self._setup_style_combo_boxes(draw_style, self._view.draw_style, VALID_DRAW_STYLE)

        line_width = float(self._model.get_line_width())
        self._view.line_width.setValue(line_width)

    def setup_marker_group(self):
        marker_style = self._model.get_marker_style()
        self._setup_style_combo_boxes(marker_style, self._view.marker_style, MARKER_STYLES.keys())

        marker_size = float(self._model.get_marker_size())
        self._view.marker_size.setValue(marker_size)

    def setup_error_bar_group(self):
        error_width = float(self._model.get_error_width())
        capsize = float(self._model.get_capsize())
        cap_thickness = float(self._model.get_cap_thickness())
        error_every = int(self._model.get_error_every())

        self._view.error_width.setValue(error_width)
        self._view.capsize.setValue(capsize)
        self._view.cap_thickness.setValue(cap_thickness)
        self._view.error_every.setValue(error_every)

    def setup_legend_group(self):
        legend_location = self._model.get_legend_location()
        self._view.legend_location.addItems(self.LEGEND_LOCATION_LIST)
        if legend_location in self.LEGEND_LOCATION_LIST:
            self._view.legend_location.setCurrentIndex(self._view.legend_location.findText(legend_location))
        legend_font_size = float(self._model.get_legend_font_size())
        self._view.legend_font_size.setValue(legend_font_size)

    def setup_images_group(self):
        colormap = self._model.get_color_map()
        if self._view.default_colormap_combo_box.findText(colormap) != -1:
            self._view.default_colormap_combo_box.setCurrentIndex(self._view.default_colormap_combo_box.findText(colormap))
            self._view.reverse_colormap_check_box.setChecked(False)
        elif colormap.endswith("_r") and self._view.default_colormap_combo_box.findText(colormap[:-2]):
            self._view.default_colormap_combo_box.setCurrentIndex(self._view.default_colormap_combo_box.findText(colormap[:-2]))
            self._view.reverse_colormap_check_box.setChecked(True)
        colorbar_scale = self._model.get_colorbar_scale()
        self._setup_style_combo_boxes(colorbar_scale, self._view.colorbar_scale, self.AXES_SCALE)

    @staticmethod
    def _setup_style_combo_boxes(current_style, style_combo, combo_items):
        if current_style in combo_items:
            style_combo.setCurrentIndex(style_combo.findText(current_style))
        else:
            style_combo.setCurrentIndex(0)

    def setup_signals(self):
        self._view.normalize_to_bin_width.stateChanged.connect(self.action_normalization_changed)
        self._view.show_title.stateChanged.connect(self.action_show_title_changed)
        self._view.show_legend.stateChanged.connect(self.action_show_legend_changed)

        # Axes
        self._view.x_axes_scale.currentTextChanged.connect(self.action_default_x_axes_changed)
        self._view.y_axes_scale.currentTextChanged.connect(self.action_default_y_axes_changed)
        self._view.axes_line_width.valueChanged.connect(self.action_axes_line_width_changed)

        # Lines
        self._view.line_style.currentTextChanged.connect(self.action_line_style_changed)
        self._view.draw_style.currentTextChanged.connect(self.action_draw_style_changed)
        self._view.line_width.valueChanged.connect(self.action_line_width_changed)
        self._view.x_min.valueChanged.connect(self.action_x_min_changed)
        self._view.x_max.valueChanged.connect(self.action_x_max_changed)
        self._view.y_min.valueChanged.connect(self.action_y_min_changed)
        self._view.y_max.valueChanged.connect(self.action_y_max_changed)
        self._view.x_min_box.stateChanged.connect(self.action_x_min_box_changed)
        self._view.x_max_box.stateChanged.connect(self.action_x_max_box_changed)
        self._view.y_min_box.stateChanged.connect(self.action_y_min_box_changed)
        self._view.y_max_box.stateChanged.connect(self.action_y_max_box_changed)

        # Ticks
        self._view.enable_grid.stateChanged.connect(self.action_enable_grid_changed)
        self._view.show_minor_ticks.stateChanged.connect(self.action_show_minor_ticks_changed)
        self._view.show_minor_gridlines.stateChanged.connect(self.action_show_minor_gridlines_changed)
        self._view.show_ticks_left.stateChanged.connect(self.action_show_ticks_left_changed)
        self._view.show_ticks_bottom.stateChanged.connect(self.action_show_ticks_bottom_changed)
        self._view.show_ticks_right.stateChanged.connect(self.action_show_ticks_right_changed)
        self._view.show_ticks_top.stateChanged.connect(self.action_show_ticks_top_changed)
        self._view.show_labels_left.stateChanged.connect(self.action_show_labels_left_changed)
        self._view.show_labels_bottom.stateChanged.connect(self.action_show_labels_bottom_changed)
        self._view.show_labels_right.stateChanged.connect(self.action_show_labels_right_changed)
        self._view.show_labels_top.stateChanged.connect(self.action_show_labels_top_changed)
        self._view.major_ticks_length.valueChanged.connect(self.action_major_ticks_length_changed)
        self._view.major_ticks_width.valueChanged.connect(self.action_major_ticks_width_changed)
        self._view.major_ticks_direction.currentTextChanged.connect(self.action_major_ticks_direction_changed)
        self._view.minor_ticks_length.valueChanged.connect(self.action_minor_ticks_length_changed)
        self._view.minor_ticks_width.valueChanged.connect(self.action_minor_ticks_width_changed)
        self._view.minor_ticks_direction.currentTextChanged.connect(self.action_minor_ticks_direction_changed)

        # Markers
        self._view.marker_style.currentTextChanged.connect(self.action_marker_style_changed)
        self._view.marker_size.valueChanged.connect(self.action_marker_size_changed)

        # Error bars
        self._view.error_width.valueChanged.connect(self.action_error_width_changed)
        self._view.capsize.valueChanged.connect(self.action_capsize_changed)
        self._view.cap_thickness.valueChanged.connect(self.action_cap_thickness_changed)
        self._view.error_every.valueChanged.connect(self.action_error_every_changed)

        # Legend
        self._view.legend_location.currentTextChanged.connect(self.action_legend_location_changed)
        self._view.legend_font_size.valueChanged.connect(self.action_legend_size_changed)

        # Images
        self._view.default_colormap_combo_box.currentTextChanged.connect(self.action_default_colormap_changed)
        self._view.reverse_colormap_check_box.stateChanged.connect(self.action_default_colormap_changed)
        self._view.plot_font.currentTextChanged.connect(self.action_font_combo_changed)
        self._view.colorbar_scale.currentTextChanged.connect(self.action_colorbar_scale_changed)

    def action_normalization_changed(self, state):
        self._model.set_normalize_by_bin_width("On" if state == Qt.Checked else "Off")

    def action_show_title_changed(self, state):
        self._model.set_show_title("On" if state == Qt.Checked else "Off")

    def action_enable_grid_changed(self, state):
        self._model.set_enable_grid("On" if state == Qt.Checked else "Off")

    def action_show_minor_ticks_changed(self, state):
        self._model.set_show_minor_ticks("On" if state == Qt.Checked else "Off")
        self._view.show_minor_gridlines.setEnabled(state == Qt.Checked)

        if not self._view.show_minor_gridlines.isEnabled():
            self._view.show_minor_gridlines.setChecked(False)

    def action_show_minor_gridlines_changed(self, state):
        self._model.set_show_minor_gridlines("On" if state == Qt.Checked else "Off")

    def action_font_combo_changed(self, font_name):
        self._model.set_plot_font(font_name)

    def action_show_legend_changed(self, state):
        self._model.set_show_legend("On" if state == Qt.Checked else "Off")

    def action_default_x_axes_changed(self, axes_scale):
        self._model.set_x_axes_scale(axes_scale)

    def action_default_y_axes_changed(self, axes_scale):
        self._model.set_y_axes_scale(axes_scale)

    def action_axes_line_width_changed(self, width):
        self._model.set_axes_line_width(str(width))

    def action_show_ticks_left_changed(self, state):
        self._model.set_show_ticks_left("On" if state == Qt.Checked else "Off")

    def action_show_ticks_bottom_changed(self, state):
        self._model.set_show_ticks_bottom("On" if state == Qt.Checked else "Off")

    def action_show_ticks_right_changed(self, state):
        self._model.set_show_ticks_right("On" if state == Qt.Checked else "Off")

    def action_show_ticks_top_changed(self, state):
        self._model.set_show_ticks_top("On" if state == Qt.Checked else "Off")

    def action_show_labels_left_changed(self, state):
        self._model.set_show_labels_left("On" if state == Qt.Checked else "Off")

    def action_show_labels_bottom_changed(self, state):
        self._model.set_show_labels_bottom("On" if state == Qt.Checked else "Off")

    def action_show_labels_right_changed(self, state):
        self._model.set_show_labels_right("On" if state == Qt.Checked else "Off")

    def action_show_labels_top_changed(self, state):
        self._model.set_show_labels_top("On" if state == Qt.Checked else "Off")

    def action_major_ticks_length_changed(self, value):
        self._model.set_major_ticks_length(str(value))

    def action_major_ticks_width_changed(self, value):
        self._model.set_major_ticks_width(str(value))

    def action_major_ticks_direction_changed(self, direction):
        self._model.set_major_ticks_direction(direction)

    def action_minor_ticks_length_changed(self, value):
        self._model.set_minor_ticks_length(str(value))

    def action_minor_ticks_width_changed(self, value):
        self._model.set_minor_ticks_width(str(value))

    def action_minor_ticks_direction_changed(self, direction):
        self._model.set_minor_ticks_direction(direction)

    def action_line_style_changed(self, style):
        self._model.set_line_style(style)

    def action_draw_style_changed(self, style):
        self._model.set_draw_style(style)

    def action_line_width_changed(self, value):
        self._model.set_line_width(str(value))

    def action_x_min_changed(self, value):
        self._model.set_x_min(str(value))

    def action_x_max_changed(self, value):
        self._model.set_x_max(str(value))

    def action_y_min_changed(self, value):
        self._model.set_y_min(str(value))

    def action_y_max_changed(self, value):
        self._model.set_y_max(str(value))

    def action_x_min_box_changed(self, state):
        self._view.x_min.setEnabled(state)
        if state:
            self.action_x_min_changed(self._view.x_min.value())
        else:
            self.action_x_min_changed("")

    def action_x_max_box_changed(self, state):
        self._view.x_max.setEnabled(state)
        if state:
            self.action_x_max_changed(self._view.x_max.value())
        else:
            self.action_x_max_changed("")

    def action_y_min_box_changed(self, state):
        self._view.y_min.setEnabled(state)
        if state:
            self.action_y_min_changed(self._view.y_min.value())
        else:
            self.action_y_min_changed("")

    def action_y_max_box_changed(self, state):
        self._view.y_max.setEnabled(state)
        if state:
            self.action_y_max_changed(self._view.y_max.value())
        else:
            self.action_y_max_changed("")

    def action_marker_style_changed(self, style):
        self._model.set_marker_style(style)

    def action_marker_size_changed(self, value):
        self._model.set_marker_size(str(value))

    def action_error_width_changed(self, value):
        self._model.set_error_width(str(value))

    def action_capsize_changed(self, value):
        self._model.set_capsize(str(value))

    def action_cap_thickness_changed(self, value):
        self._model.set_cap_thickness(str(value))

    def action_error_every_changed(self, value):
        self._model.set_error_every(str(value))

    def action_legend_location_changed(self, location):
        self._model.set_legend_location(str(location))

    def action_legend_size_changed(self, value):
        self._model.set_legend_font_size(str(value))

    def action_colorbar_scale_changed(self, value):
        self._model.set_colorbar_scale(value)

    def action_default_colormap_changed(self):
        colormap = self._view.default_colormap_combo_box.currentText()
        if self._view.reverse_colormap_check_box.isChecked():
            colormap += "_r"

        self._model.set_color_map(colormap)

    def populate_font_combo_box(self):
        fonts = self._model.get_font_names()
        self._view.plot_font.addItems(fonts)
        current_font = self._model.get_plot_font()
        if current_font:
            self._view.plot_font.setCurrentText(current_font)
        else:
            self._view.plot_font.setCurrentText(self._model.get_current_mpl_font())

    def update_properties(self):
        self.load_general_setting_values()
        self.setup_axes_group()
        self.setup_line_group()
        self.setup_marker_group()
        self.setup_error_bar_group()
        self.setup_legend_group()
        self.setup_images_group()
