# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

import matplotlib.axis
from matplotlib import ticker
from matplotlib.image import AxesImage

from mantid import logger
from mantid.plots import MantidAxKwargs

try:
    from matplotlib.colors import to_hex
except ImportError:
    from matplotlib.colors import colorConverter, rgb2hex

    def to_hex(color):
        return rgb2hex(colorConverter.to_rgb(color))


class PlotsSaver(object):
    def __init__(self):
        self.figure_creation_args = {}

    def save_plots(self, plot_dict):
        # if arguement is none return empty dictionary
        if plot_dict is None:
            return []

        plot_list = []
        for index in plot_dict:
            try:
                plot_list.append(self.get_dict_from_fig(plot_dict[index].canvas.figure))
            except BaseException as e:
                # Catch all errors in here so it can fail silently-ish, if this is happening on all plots make sure you
                # have built your project.
                if isinstance(e, KeyboardInterrupt):
                    raise KeyboardInterrupt
                logger.warning("A plot was unable to be saved")
        return plot_list

    def get_dict_from_fig(self, fig):
        axes_list = []
        create_list = []
        for ax in fig.axes:
            try:
                create_list.append(ax.creation_args)
                self.figure_creation_args = ax.creation_args
            except AttributeError:
                logger.debug("Axis had a axis without creation_args - Common with colorfill")
                continue
            axes_list.append(self.get_dict_for_axes(ax))

        fig_dict = {"creationArguments": create_list,
                    "axes": axes_list,
                    "label": fig._label,
                    "properties": self.get_dict_from_fig_properties(fig)}
        return fig_dict

    @staticmethod
    def get_dict_for_axes_colorbar(ax):
        image = None
        cb_dict = {}

        # If an image is present (from imshow)
        if len(ax.images) > 0 and isinstance(ax.images[0], AxesImage):
            image = ax.images[0]
        # If an image is present from pcolor/pcolormesh
        elif len(ax.collections) > 0 and isinstance(ax.collections[0], AxesImage):
            image = ax.collections[0]
        else:
            cb_dict["exists"] = False
            return cb_dict

        cb_dict["exists"] = True
        cb_dict["max"] = image.norm.vmax
        cb_dict["min"] = image.norm.vmin
        cb_dict["interpolation"] = image._interpolation
        cb_dict["cmap"] = image.cmap.name
        cb_dict["label"] = image._label

        return cb_dict

    def get_dict_for_axes(self, ax):
        ax_dict = {"properties": self.get_dict_from_axes_properties(ax),
                   "title": ax.get_title(),
                   "xAxisTitle": ax.get_xlabel(),
                   "yAxisTitle": ax.get_ylabel(),
                   "colorbar": self.get_dict_for_axes_colorbar(ax)}

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
        prop_dict = {"majorTickLocator": type(ax.get_major_locator()).__name__,
                     "minorTickLocator": type(ax.get_minor_locator()).__name__,
                     "majorTickFormatter": type(ax.get_major_formatter()).__name__,
                     "minorTickFormatter": type(ax.get_minor_formatter()).__name__,
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
            grid_style["color"] = to_hex(gridlines[0].get_color())
            grid_style["alpha"] = gridlines[0].get_alpha()
            grid_style["gridOn"] = True
        else:
            grid_style["gridOn"] = False
        return grid_style

    def get_dict_from_line(self, line, index=0):
        line_dict = {"lineIndex": index,
                     "label": line.get_label(),
                     "alpha": line.get_alpha(),
                     "color": to_hex(line.get_color()),
                     "lineWidth": line.get_linewidth(),
                     "lineStyle": line.get_linestyle(),
                     "markerStyle": self.get_dict_from_marker_style(line),
                     "errorbars": self.get_dict_for_errorbars(line)}
        if line_dict["alpha"] is None:
            line_dict["alpha"] = 1
        return line_dict

    def get_dict_for_errorbars(self, line):
        cargs = self.figure_creation_args[0]
        if cargs["function"] == "errorbar" or (
                MantidAxKwargs.POST_CREATION_ARGS in cargs
                and cargs[MantidAxKwargs.POST_CREATION_ARGS][MantidAxKwargs.ERRORS_ADDED]):
            return {"exists": True,
                    "dashCapStyle": line.get_dash_capstyle(),
                    "dashJoinStyle": line.get_dash_joinstyle(),
                    "solidCapStyle": line.get_solid_capstyle(),
                    "solidJoinStyle": line.get_solid_joinstyle(),
                    "visible": cargs[MantidAxKwargs.POST_CREATION_ARGS][MantidAxKwargs.ERRORS_VISIBLE]}
        else:
            return {"exists": False}

    @staticmethod
    def get_dict_from_marker_style(line):
        style_dict = {"faceColor": to_hex(line.get_markerfacecolor()),
                      "edgeColor": to_hex(line.get_markeredgecolor()),
                      "edgeWidth": line.get_markeredgewidth(),
                      "markerType": line.get_marker(),
                      "markerSize": line.get_markersize(),
                      "zOrder": line.get_zorder()}
        return style_dict

    def get_dict_from_text(self, text):
        text_dict = {"text": text.get_text()}
        if text_dict["text"]:
            # text_dict["transform"] = text.get_transform()
            text_dict["position"] = text.get_position()
            text_dict["useTeX"] = text.get_usetex()
            text_dict["style"] = self.get_dict_from_text_style(text)
        return text_dict

    @staticmethod
    def get_dict_from_text_style(text):
        style_dict = {"alpha": text.get_alpha(),
                      "textSize": text.get_size(),
                      "color": to_hex(text.get_color()),
                      "hAlign": text.get_horizontalalignment(),
                      "vAlign": text.get_verticalalignment(),
                      "rotation": text.get_rotation(),
                      "zOrder": text.get_zorder()}
        if style_dict["alpha"] is None:
            style_dict["alpha"] = 1
        return style_dict

    @staticmethod
    def get_dict_from_fig_properties(fig):
        return {"figWidth": fig.get_figwidth(), "figHeight": fig.get_figheight(), "dpi": fig.dpi}
