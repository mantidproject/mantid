# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

from matplotlib import ticker
import matplotlib.colors


global_args = {}


class PlotsSaver(object):
    def save_plots(self, plots):
        plot_list = []
        for plot in plots:
            (plot.num ,plot_list.append(self.get_dict_from_fig(plot.canvas.figure)))
        return plot_list

    def get_dict_from_fig(self, fig):
        fig_dict = {"creationArguements": global_args}

        axes_list = []
        for ax in fig.axes:
            axes_list.append(self.get_dict_for_axes(ax))
        fig_dict["axes"] = axes_list

        fig_dict["properties"] = self.get_dict_from_fig_properties(fig)
        return fig_dict

    def get_dict_for_axes(self, ax):
        ax_dict = {"properties": self.get_dict_from_axes_properties(ax),
                   "title": self.get_dict_from_text(ax.title),
                   "xAxisTitle": self.get_dict_from_text(ax.xaxis.label),
                   "yAxisTitle": self.get_dict_from_text(ax.yaxis.label)}

        # Get lines from the axes and store it's data
        lines_list = []
        for index, line in enumerate(ax.lines):
            lines_list.append(self.get_dict_from_line(line, index))
        ax_dict["lines"] = lines_list

        texts_list = []
        for text in ax.texts:
            texts_list.append(self.get_dict_from_text(text))
        ax_dict["texts"] = texts_list

        # Potentially need to handle artists that are Text
        artist_text_dict = {}
        for artist in ax.artists:
            if isinstance(artist, matplotlib.text.Text):
                artist_text_dict = self.get_dict_from_text(artist)
        ax_dict["textFromArtists"] = artist_text_dict

        legend_dict = {}
        legend = ax.get_legend()
        if legend is not None:
            legend_dict["visible"] = legend.get_visible()
            legend_dict["exists"] = True
        else:
            legend_dict["exists"] = False
        ax_dict["legend"] = legend_dict

        return ax_dict

    def get_dict_from_axes_properties(self, ax):
        return {"bounds": ax.get_position().bounds,
                "dynamic": ax.get_navigate(),
                "axisOn": ax.axison,
                "frameOn": ax.get_frame_on(),
                "visible": ax.get_visible(),
                "xAxisProperties": self.get_dict_from_axis_properties(ax.xaxis),
                "yAxisProperties": self.get_dict_from_axis_properties(ax.yaxis),
                "xAxisScale": ax.xaxis.get_scale(),
                "xLim": ax.get_xlim(),
                "yAxisScale": ax.yaxis.get_scale(),
                "yLim": ax.get_ylim()}

    def get_dict_from_axis_properties(self, ax):
        prop_dict = {"majorticklocator": type(ax.get_major_locator()).__name__,
                     "minorticklocator": type(ax.get_minor_locator()).__name__,
                     "majortickformatter": type(ax.get_major_formatter()).__name__,
                     "minortickformatter": type(ax.get_minor_formatter()).__name__,
                     "gridStyle": self.get_dict_for_grid_style(ax),
                     "visible": ax.get_visible()}
        label1On = ax._major_tick_kw.get('label1On', True)

        if isinstance(ax, matplotlib.axis.XAxis):
            if label1On:
                prop_dict["position"] = "Bottom"
            else:
                prop_dict["position"] = "Top"
        elif isinstance(ax, matplotlib.axis.YAxis):
            if label1On:
                prop_dict["position"] = "Left"
            else:
                prop_dict["position"] = "Right"
        else:
            raise ValueError("Value passed is not a valid axis")

        if isinstance(ax.get_major_locator(), ticker.FixedLocator):
            prop_dict["majorTickLocatorValues"] = list(ax.get_major_locator())
        else:
            prop_dict["majorTickLocatorValues"] = None

        if isinstance(ax.get_minor_locator(), ticker.FixedLocator):
            prop_dict["minorTickLocatorValues"] = list(ax.get_minor_locator())
        else:
            prop_dict["minorTickLocatorValues"] = None

        formatter = ax.get_major_formatter()
        if isinstance(formatter, ticker.FixedFormatter):
            prop_dict["majorTickFormat"] = list(formatter.seq)
        else:
            prop_dict["majorTickFormat"] = None

        formatter = ax.get_minor_formatter()
        if isinstance(formatter, ticker.FixedFormatter):
            prop_dict["minorTickFormat"] = list(formatter.seq)
        else:
            prop_dict["minorTickFormat"] = None

        labels = ax.get_ticklabels()
        if labels:
            prop_dict["fontSize"] = labels[0].get_fontsize()
        else:
            prop_dict["fontSize"] = ""

        return prop_dict

    @staticmethod
    def get_dict_for_grid_style(ax):
        grid_style = {}
        gridlines = ax.get_gridlines()
        if ax._gridOnMajor and len(gridlines) > 0:
            grid_style["color"] = matplotlib.colors.to_hex(gridlines[0].get_color())
            grid_style["alpha"] = gridlines[0].get_alpha()
            grid_style["gridOn"] = True
        else:
            grid_style["gridOn"] = False
        return grid_style

    def get_dict_from_line(self, line, index=0):
        line_dict = {"lineIndex": index,
                     "label": line.get_label(),
                     "alpha": line.get_alpha(),
                     "color": matplotlib.colors.to_hex(line.get_color()),
                     "lineWidth": line.get_linewidth(),
                     "lineStyle": line.get_linestyle(),
                     "markerStyle": self.get_dict_from_marker_style(line)}
        if line_dict["alpha"] is None:
            line_dict["alpha"] = 1
        return line_dict

    @staticmethod
    def get_dict_from_marker_style(line):
        style_dict = {"faceColor": matplotlib.colors.to_hex(line.get_markerfacecolor()),
                      "edgeColor": matplotlib.colors.to_hex(line.get_markeredgecolor()),
                      "edgeWidth": line.get_markeredgewidth(),
                      "markerType": line.get_marker(),
                      "markerSize": line.get_markersize(),
                      "zOrder": line.get_zorder()}
        return style_dict

    def get_dict_from_text(self, text):
        text_dict = {"text": text.get_text()}
        if text_dict["text"]:
            text_dict["transform"] = text.get_transform()
            text_dict["position"] = text.get_position()
            text_dict["style"] = self.get_dict_from_text_style(text)
        return text_dict

    @staticmethod
    def get_dict_from_text_style(text):
        style_dict = {"alpha": text.get_alpha(),
                      "textSize": text.get_size(),
                      "color": matplotlib.colors.to_hex(text.get_color()),
                      "hAlign": text.get_horizontalalignment(),
                      "vAlign": text.get_verticalalignment(),
                      "mAlign": text._multialignment,
                      "rotation": text.get_rotation(),
                      "zOrder": text.get_zorder()}
        if style_dict["alpha"] is None:
            style_dict["alpha"] = 1
        return style_dict

    def get_list_of_legend_children(self, legend):
        legend_list = []
        if hasattr(legend, 'get_children') and len(legend.get_children()) > 0:
            for child in legend.get_children():
                legend_list.append(self.get_list_of_legend_children(child))
        else:
            legend_list.append(legend)
        return legend_list

    @staticmethod
    def get_dict_from_fig_properties(fig):
        return {"figWidth": fig.get_figwidth(), "figHeight": fig.get_figheight(), "dpi": fig.dpi}


def plot_decorator(func):
    def wrapper(*args, **kwargs):
        global global_args
        global_args.append(args, kwargs)
        return func(*args, **kwargs)
    return wrapper
