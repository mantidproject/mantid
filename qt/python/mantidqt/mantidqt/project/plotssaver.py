# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from copy import deepcopy
import matplotlib.axis
from matplotlib import ticker
from matplotlib.image import AxesImage

from mantid import logger
from mantid.plots.legend import LegendProperties
from mantid.plots.utility import MantidAxType

from matplotlib.colors import to_hex, Normalize


class PlotsSaver(object):
    def __init__(self):
        self.figure_creation_args = {}

    def save_plots(self, plot_dict, is_project_recovery=False):
        # if argument is none return empty dictionary
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

                error_string = "Plot: " + str(index) + " was not saved. Error: " + str(e)
                if not is_project_recovery:
                    logger.warning(error_string)
                else:
                    logger.debug(error_string)
        return plot_list

    @staticmethod
    def _convert_normalise_obj_to_dict(norm):
        norm_dict = {'type': type(norm).__name__, 'clip': norm.clip, 'vmin': norm.vmin, 'vmax': norm.vmax}
        return norm_dict

    @staticmethod
    def _add_normalisation_kwargs(cargs_list, axes_list):
        for ax_cargs, ax_dict in zip(cargs_list[0], axes_list):
            is_norm = ax_dict.pop("_is_norm")
            ax_cargs['normalize_by_bin_width'] = is_norm

    def get_dict_from_fig(self, fig):
        axes_list = []
        create_list = []
        for ax in fig.axes:
            try:
                creation_args = deepcopy(ax.creation_args)
                # convert the normalise object (if present) into a dict so that it can be json serialised
                for args_dict in creation_args:
                    if 'axis' in args_dict and type(args_dict['axis']) is MantidAxType:
                        args_dict['axis'] = args_dict['axis'].value
                    if 'norm' in args_dict.keys() and isinstance(args_dict['norm'], Normalize):
                        norm_dict = self._convert_normalise_obj_to_dict(args_dict['norm'])
                        args_dict['norm'] = norm_dict
                    if 'axis' in args_dict.keys():
                        args_dict['axis'] = args_dict['axis'].value
                create_list.append(creation_args)
                self.figure_creation_args = creation_args
            except AttributeError:
                logger.debug("Axis had an axis without creation_args - Common with a Colorfill plot")
                continue
            axes_list.append(self.get_dict_for_axes(ax))

        if create_list and axes_list:
            self._add_normalisation_kwargs(create_list, axes_list)
        fig_dict = {"creationArguments": create_list,
                    "axes": axes_list,
                    "label": fig._label,
                    "properties": self.get_dict_from_fig_properties(fig)}
        return fig_dict

    @staticmethod
    def get_dict_for_axes_colorbar(ax):
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
            legend_dict["exists"] = True
            legend_dict.update(LegendProperties.from_legend(legend))
        else:
            legend_dict["exists"] = False
        ax_dict["legend"] = legend_dict

        # add value to determine if ax has been normalised
        ws_artists = [art for art in ax.tracked_workspaces.values()]
        is_norm = all(art[0].is_normalized for art in ws_artists)
        ax_dict["_is_norm"] = is_norm

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
                "xAutoScale": ax.get_autoscalex_on(),
                "yAxisScale": ax.yaxis.get_scale(),
                "yLim": ax.get_ylim(),
                "yAutoScale": ax.get_autoscaley_on(),
                "showMinorGrid": hasattr(ax, 'show_minor_gridlines') and ax.show_minor_gridlines,
                "tickParams": self.get_dict_from_tick_properties(ax),
                "spineWidths": self.get_dict_from_spine_widths(ax)}

    def get_dict_from_axis_properties(self, ax):
        prop_dict = {"majorTickLocator": type(ax.get_major_locator()).__name__,
                     "minorTickLocator": type(ax.get_minor_locator()).__name__,
                     "majorTickFormatter": type(ax.get_major_formatter()).__name__,
                     "minorTickFormatter": type(ax.get_minor_formatter()).__name__,
                     "gridStyle": self.get_dict_for_grid_style(ax),
                     "visible": ax.get_visible()}

        if not (isinstance(ax, matplotlib.axis.YAxis) or isinstance(ax, matplotlib.axis.XAxis)):
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
            grid_style["minorGridOn"] = ax._gridOnMinor
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
        if self.figure_creation_args[0]["function"] == "errorbar":
            return {"exists": True,
                    "dashCapStyle": line.get_dash_capstyle(),
                    "dashJoinStyle": line.get_dash_joinstyle(),
                    "solidCapStyle": line.get_solid_capstyle(),
                    "solidJoinStyle": line.get_solid_joinstyle()}
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

    @staticmethod
    def get_dict_from_tick_properties(ax):
        xaxis_major_kw = ax.xaxis._major_tick_kw
        xaxis_minor_kw = ax.xaxis._minor_tick_kw

        yaxis_major_kw = ax.yaxis._major_tick_kw
        yaxis_minor_kw = ax.yaxis._minor_tick_kw

        tick_dict = {
            "xaxis": {
                "major": {
                    "bottom": xaxis_major_kw['tick1On'],
                    "top": xaxis_major_kw['tick2On'],
                    "labelbottom": xaxis_major_kw['label1On'],
                    "labeltop": xaxis_major_kw['label2On']
                },
                "minor": {
                    "bottom": xaxis_minor_kw['tick1On'],
                    "top": xaxis_minor_kw['tick2On'],
                    "labelbottom": xaxis_minor_kw['label1On'],
                    "labeltop": xaxis_minor_kw['label2On']
                }
            },
            "yaxis": {
                "major": {
                    "left": yaxis_major_kw['tick1On'],
                    "right": yaxis_major_kw['tick2On'],
                    "labelleft": yaxis_major_kw['label1On'],
                    "labelright": yaxis_major_kw['label2On']
                },
                "minor": {
                    "left": yaxis_minor_kw['tick1On'],
                    "right": yaxis_minor_kw['tick2On'],
                    "labelleft": yaxis_minor_kw['label1On'],
                    "labelright": yaxis_minor_kw['label2On']
                }
            }
        }
        # Set none guaranteed variables in tick_dict
        for axis in tick_dict:
            for size in tick_dict[axis]:
                # Setup keyword dict for given axis and size (major/minor)
                keyword_dict_variable_name = f"{axis}_{size}_kw"
                keyword_dict = locals()[keyword_dict_variable_name]

                if "tickdir" in keyword_dict:
                    tick_dict[axis][size]["direction"] = keyword_dict["tickdir"]
                if "size" in keyword_dict:
                    tick_dict[axis][size]["size"] = keyword_dict["size"]
                if "width" in keyword_dict:
                    tick_dict[axis][size]["width"] = keyword_dict["width"]

        return tick_dict

    @staticmethod
    def get_dict_from_spine_widths(ax):
        return {
            'left': ax.spines['left']._linewidth,
            'right': ax.spines['right']._linewidth,
            'bottom': ax.spines['bottom']._linewidth,
            'top': ax.spines['top']._linewidth}
