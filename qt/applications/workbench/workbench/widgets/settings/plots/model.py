# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
from enum import Enum

from matplotlib import font_manager
import matplotlib as mpl

from workbench.widgets.settings.base_classes.config_settings_changes_model import ConfigSettingsChangesModel


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


class PlotsSettingsModel(ConfigSettingsChangesModel):
    def get_font_names(self):
        font_list = font_manager.findSystemFonts()
        fonts = set()
        for font in font_list:
            # This try-except is for a known matplotlib bug where get_name() causes an error for certain fonts.
            try:
                font_name = font_manager.FontProperties(fname=font).get_name()
            except RuntimeError:
                continue
            fonts.add(font_name)
        fonts.add(self.get_current_mpl_font())
        fonts = sorted(fonts)
        return fonts

    @staticmethod
    def get_current_mpl_font():
        if mpl.rcParams["font.family"][0] in ["sans-serif", "serif", "cursive", "fantasy", "monospace"]:
            current_mpl_font = mpl.rcParams["font." + mpl.rcParams["font.family"][0]][0]
        else:
            current_mpl_font = mpl.rcParams["font.family"][0]
        return current_mpl_font

    def get_normalize_to_bin_width(self) -> str:
        return self.get_saved_value(PlotProperties.NORMALIZATION.value)

    def get_show_title(self) -> str:
        return self.get_saved_value(PlotProperties.SHOW_TITLE.value)

    def get_show_legend(self) -> str:
        return self.get_saved_value(PlotProperties.SHOW_LEGEND.value)

    def get_x_axes_scale(self) -> str:
        return self.get_saved_value(PlotProperties.X_AXES_SCALE.value)

    def get_y_axes_scale(self) -> str:
        return self.get_saved_value(PlotProperties.Y_AXES_SCALE.value)

    def get_axes_line_width(self) -> str:
        return self.get_saved_value(PlotProperties.AXES_LINE_WIDTH.value)

    def get_x_min(self) -> str:
        return self.get_saved_value(PlotProperties.X_MIN.value)

    def get_x_max(self) -> str:
        return self.get_saved_value(PlotProperties.X_MAX.value)

    def get_y_min(self) -> str:
        return self.get_saved_value(PlotProperties.Y_MIN.value)

    def get_y_max(self) -> str:
        return self.get_saved_value(PlotProperties.Y_MAX.value)

    def get_show_ticks_left(self) -> str:
        return self.get_saved_value(PlotProperties.SHOW_TICKS_LEFT.value)

    def get_show_ticks_bottom(self) -> str:
        return self.get_saved_value(PlotProperties.SHOW_TICKS_BOTTOM.value)

    def get_show_ticks_right(self) -> str:
        return self.get_saved_value(PlotProperties.SHOW_TICKS_RIGHT.value)

    def get_show_ticks_top(self) -> str:
        return self.get_saved_value(PlotProperties.SHOW_TICKS_TOP.value)

    def get_show_labels_left(self) -> str:
        return self.get_saved_value(PlotProperties.SHOW_LABELS_LEFT.value)

    def get_show_labels_bottom(self) -> str:
        return self.get_saved_value(PlotProperties.SHOW_LABELS_BOTTOM.value)

    def get_show_labels_right(self) -> str:
        return self.get_saved_value(PlotProperties.SHOW_LABELS_RIGHT.value)

    def get_show_labels_top(self) -> str:
        return self.get_saved_value(PlotProperties.SHOW_LABELS_TOP.value)

    def get_major_ticks_length(self) -> str:
        return self.get_saved_value(PlotProperties.MAJOR_TICKS_LENGTH.value)

    def get_major_ticks_width(self) -> str:
        return self.get_saved_value(PlotProperties.MAJOR_TICKS_WIDTH.value)

    def get_major_ticks_direction(self) -> str:
        return self.get_saved_value(PlotProperties.MAJOR_TICKS_DIRECTION.value)

    def get_minor_ticks_length(self) -> str:
        return self.get_saved_value(PlotProperties.MINOR_TICKS_LENGTH.value)

    def get_minor_ticks_width(self) -> str:
        return self.get_saved_value(PlotProperties.MINOR_TICKS_WIDTH.value)

    def get_minor_ticks_direction(self) -> str:
        return self.get_saved_value(PlotProperties.MINOR_TICKS_DIRECTION.value)

    def get_enable_grid(self) -> str:
        return self.get_saved_value(PlotProperties.ENABLE_GRID.value)

    def get_show_minor_ticks(self) -> str:
        return self.get_saved_value(PlotProperties.SHOW_MINOR_TICKS.value)

    def get_show_minor_gridlines(self) -> str:
        return self.get_saved_value(PlotProperties.SHOW_MINOR_GRIDLINES.value)

    def get_plot_font(self) -> str:
        return self.get_saved_value(PlotProperties.PLOT_FONT.value)

    def get_line_style(self) -> str:
        return self.get_saved_value(PlotProperties.LINE_STYLE.value)

    def get_draw_style(self) -> str:
        return self.get_saved_value(PlotProperties.DRAW_STYLE.value)

    def get_line_width(self) -> str:
        return self.get_saved_value(PlotProperties.LINE_WIDTH.value)

    def get_marker_style(self) -> str:
        return self.get_saved_value(PlotProperties.MARKER_STYLE.value)

    def get_marker_size(self) -> str:
        return self.get_saved_value(PlotProperties.MARKER_SIZE.value)

    def get_error_width(self) -> str:
        return self.get_saved_value(PlotProperties.ERROR_WIDTH.value)

    def get_capsize(self) -> str:
        return self.get_saved_value(PlotProperties.CAPSIZE.value)

    def get_cap_thickness(self) -> str:
        return self.get_saved_value(PlotProperties.CAP_THICKNESS.value)

    def get_error_every(self) -> str:
        return self.get_saved_value(PlotProperties.ERROR_EVERY.value)

    def get_legend_location(self) -> str:
        return self.get_saved_value(PlotProperties.LEGEND_LOCATION.value)

    def get_legend_font_size(self) -> str:
        return self.get_saved_value(PlotProperties.LEGEND_FONT_SIZE.value)

    def get_color_map(self) -> str:
        return self.get_saved_value(PlotProperties.COLORMAP.value)

    def get_colorbar_scale(self) -> str:
        return self.get_saved_value(PlotProperties.COLORBAR_SCALE.value)

    def set_normalize_by_bin_width(self, value: str) -> None:
        self.add_change(PlotProperties.NORMALIZATION.value, value)

    def set_show_title(self, value: str) -> None:
        self.add_change(PlotProperties.SHOW_TITLE.value, value)

    def set_enable_grid(self, value: str) -> None:
        self.add_change(PlotProperties.ENABLE_GRID.value, value)

    def set_show_minor_ticks(self, value: str) -> None:
        self.add_change(PlotProperties.SHOW_MINOR_TICKS.value, value)

    def set_show_minor_gridlines(self, value: str) -> None:
        self.add_change(PlotProperties.SHOW_MINOR_GRIDLINES.value, value)

    def set_plot_font(self, value: str) -> None:
        self.add_change(PlotProperties.PLOT_FONT.value, value)

    def set_show_legend(self, value: str) -> None:
        self.add_change(PlotProperties.SHOW_LEGEND.value, value)

    def set_x_axes_scale(self, value: str) -> None:
        self.add_change(PlotProperties.X_AXES_SCALE.value, value)

    def set_y_axes_scale(self, value: str) -> None:
        self.add_change(PlotProperties.Y_AXES_SCALE.value, value)

    def set_axes_line_width(self, value: str) -> None:
        self.add_change(PlotProperties.AXES_LINE_WIDTH.value, value)

    def set_show_ticks_left(self, value: str) -> None:
        self.add_change(PlotProperties.SHOW_TICKS_LEFT.value, value)

    def set_show_ticks_bottom(self, value: str) -> None:
        self.add_change(PlotProperties.SHOW_TICKS_BOTTOM.value, value)

    def set_show_ticks_right(self, value: str) -> None:
        self.add_change(PlotProperties.SHOW_TICKS_RIGHT.value, value)

    def set_show_ticks_top(self, value: str) -> None:
        self.add_change(PlotProperties.SHOW_TICKS_TOP.value, value)

    def set_show_labels_left(self, value: str) -> None:
        self.add_change(PlotProperties.SHOW_LABELS_LEFT.value, value)

    def set_show_labels_bottom(self, value: str) -> None:
        self.add_change(PlotProperties.SHOW_LABELS_BOTTOM.value, value)

    def set_show_labels_right(self, value: str) -> None:
        self.add_change(PlotProperties.SHOW_LABELS_RIGHT.value, value)

    def set_show_labels_top(self, value: str) -> None:
        self.add_change(PlotProperties.SHOW_LABELS_TOP.value, value)

    def set_major_ticks_length(self, value: str) -> None:
        self.add_change(PlotProperties.MAJOR_TICKS_LENGTH.value, value)

    def set_major_ticks_width(self, value: str) -> None:
        self.add_change(PlotProperties.MAJOR_TICKS_WIDTH.value, value)

    def set_major_ticks_direction(self, value: str) -> None:
        self.add_change(PlotProperties.MAJOR_TICKS_DIRECTION.value, value)

    def set_minor_ticks_length(self, value: str) -> None:
        self.add_change(PlotProperties.MINOR_TICKS_LENGTH.value, value)

    def set_minor_ticks_width(self, value: str) -> None:
        self.add_change(PlotProperties.MINOR_TICKS_WIDTH.value, value)

    def set_minor_ticks_direction(self, value: str) -> None:
        self.add_change(PlotProperties.MINOR_TICKS_DIRECTION.value, value)

    def set_line_style(self, value: str) -> None:
        self.add_change(PlotProperties.LINE_STYLE.value, value)

    def set_draw_style(self, value: str) -> None:
        self.add_change(PlotProperties.DRAW_STYLE.value, value)

    def set_line_width(self, value: str) -> None:
        self.add_change(PlotProperties.LINE_WIDTH.value, value)

    def set_x_min(self, value: str) -> None:
        self.add_change(PlotProperties.X_MIN.value, value)

    def set_x_max(self, value: str) -> None:
        self.add_change(PlotProperties.X_MAX.value, value)

    def set_y_min(self, value: str) -> None:
        self.add_change(PlotProperties.Y_MIN.value, value)

    def set_y_max(self, value: str) -> None:
        self.add_change(PlotProperties.Y_MAX.value, value)

    def set_marker_style(self, value: str) -> None:
        self.add_change(PlotProperties.MARKER_STYLE.value, value)

    def set_marker_size(self, value: str) -> None:
        self.add_change(PlotProperties.MARKER_SIZE.value, value)

    def set_error_width(self, value: str) -> None:
        self.add_change(PlotProperties.ERROR_WIDTH.value, value)

    def set_capsize(self, value: str) -> None:
        self.add_change(PlotProperties.CAPSIZE.value, value)

    def set_cap_thickness(self, value: str) -> None:
        self.add_change(PlotProperties.CAP_THICKNESS.value, value)

    def set_error_every(self, value: str) -> None:
        self.add_change(PlotProperties.ERROR_EVERY.value, value)

    def set_legend_location(self, value: str) -> None:
        self.add_change(PlotProperties.LEGEND_LOCATION.value, value)

    def set_legend_font_size(self, value: str) -> None:
        self.add_change(PlotProperties.LEGEND_FONT_SIZE.value, value)

    def set_colorbar_scale(self, value: str) -> None:
        self.add_change(PlotProperties.COLORBAR_SCALE.value, value)

    def set_color_map(self, value: str) -> None:
        self.add_change(PlotProperties.COLORMAP.value, value)
