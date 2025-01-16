# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
import copy

import matplotlib.axes
import matplotlib.colors
from matplotlib import axis, ticker, colormaps  # noqa
from matplotlib.ticker import NullFormatter, ScalarFormatter, LogFormatterSciNotation

from mantid import logger
from mantid.api import AnalysisDataService as ADS
from mantid.plots.legend import LegendProperties
from mantid.plots.plotfunctions import create_subplots

# Constants set in workbench.plotting.functions but would cause backwards reliability
from mantidqt.plotting.functions import pcolormesh

SUBPLOT_WSPACE = 0.5
SUBPLOT_HSPACE = 0.5

TICK_FORMATTERS = {
    "NullFormatter": NullFormatter(),
    "ScalarFormatter": ScalarFormatter(useOffset=True),
    "LogFormatterSciNotation": LogFormatterSciNotation(),
}


def get_tick_format(tick_formatters: dict, tick_formatter: str, tick_format):
    if tick_formatter == "FixedFormatter":
        fmt = ticker.FixedFormatter(tick_format)
    else:
        try:
            fmt = tick_formatters[tick_formatter]
        except KeyError:
            # If the formatter is not FixedFormatter or
            # does not exist in global_tick_format_dict,
            # default to ScalarFormatter
            fmt = tick_formatters["ScalarFormatter"]
    return fmt


class PlotsLoader(object):
    def __init__(self):
        self.color_bar_remade = False

    def load_plots(self, plots_list):
        if plots_list is None:
            return
        for plot_ in plots_list:
            try:
                self.make_fig(plot_)
            except BaseException as e:
                # Catch all errors in here so it can fail silently-ish
                if isinstance(e, KeyboardInterrupt):
                    raise KeyboardInterrupt(str(e))
                logger.warning("A plot was unable to be loaded from the save file. Error: " + str(e))

    def restore_normalise_obj_from_dict(self, norm_dict):
        supported_norm_types = {
            # matplotlib norms that are supported.
            "Normalize": matplotlib.colors.Normalize,
            "LogNorm": matplotlib.colors.LogNorm,
        }
        # If there is a norm dict, but the type is not specified, default to base Normalize class.
        type = norm_dict["type"] if "type" in norm_dict.keys() else "Normalize"

        if type not in supported_norm_types.keys():
            logger.debug(f"Color normalisation of type {norm_dict['type']} is not supported. Normalisation will not be set on this plot")
            return None

        norm = supported_norm_types[type]
        return norm(vmin=norm_dict["vmin"], vmax=norm_dict["vmax"], clip=norm_dict["clip"])

    def make_fig(self, plot_dict, create_plot=True):
        """
        This method currently only considers single matplotlib.axes.Axes based figures as that is the most common case
        :param plot_dict: dictionary; A dictionary of various items intended to recreate a figure
        :param create_plot: Bool; whether or not to make the plot, or to return the figure.
        :return: matplotlib.figure; Only returns if create_plot=False
        """  # Grab creation arguments
        creation_args = plot_dict["creationArguments"]

        if len(creation_args) == 0:
            logger.information(
                "A plot could not be loaded from the save file, as it did not have creation_args. The original plot title was: {}".format(
                    plot_dict["label"]
                )
            )
            return

        for sublist in creation_args:
            for cargs_dict in sublist:
                if "norm" in cargs_dict and isinstance(cargs_dict["norm"], dict):
                    cargs_dict["norm"] = self.restore_normalise_obj_from_dict(cargs_dict["norm"])
        fig, axes_matrix, _, _ = create_subplots(len(creation_args))
        axes_list = axes_matrix.flatten().tolist()
        for ax, cargs_list in zip(axes_list, creation_args):
            creation_args_copy = copy.deepcopy(cargs_list)
            for cargs in cargs_list:
                if "workspaces" in cargs:
                    workspace_name = cargs.pop("workspaces")
                    workspace = ADS.retrieve(workspace_name)
                    self.workspace_plot_func(workspace, ax, ax.figure, cargs)
                elif "function" in cargs:
                    self.plot_func(ax, cargs)
            for cargs in creation_args_copy:
                cargs.pop("normalize_by_bin_width", None)
            ax.creation_args = creation_args_copy

        # Update the fig
        fig._label = plot_dict["label"]
        if fig.canvas.manager is not None:
            fig.canvas.manager.set_window_title(plot_dict["label"])
        self.restore_figure_data(fig=fig, dic=plot_dict)

        # If the function should create plot then create else return
        if create_plot:
            fig.show()
        else:
            return fig

    def workspace_plot_func(self, workspace, axes, fig, creation_arg):
        """
        Plot's the graph from the given workspace, axes and creation_args. then returns the function used to create it.
        :param workspace: mantid.Workspace; Workspace to create the graph from
        :param axes: matplotlib.Axes; Axes to create the graph
        :param fig: matplotlib.Figure; Figure to add the colormesh to
        :param creation_arg: The creation arguments that have been used to create the details of the
        :return: String; The function used to create the plot
        """
        # Remove the function kwarg and if it's not found set to "plot
        function_to_call = creation_arg.pop("function")

        # Handle recreating the cmap objects
        if "cmap" in creation_arg:
            creation_arg["cmap"] = getattr(matplotlib.cm, creation_arg["cmap"])

        function_dict = {
            "plot": axes.plot,
            "scatter": axes.scatter,
            "errorbar": axes.errorbar,
            "pcolor": axes.pcolor,
            "pcolorfast": axes.pcolorfast,
            "pcolormesh": pcolormesh,
            "imshow": pcolormesh,
            "contour": axes.contour,
            "contourf": axes.contourf,
            "tripcolor": axes.tripcolor,
            "tricontour": axes.tricontour,
            "tricontourf": axes.tricontourf,
        }

        func = function_dict[function_to_call]
        # Plotting is done via an Axes object unless a colorbar needs to be added
        if function_to_call in ["imshow", "pcolormesh"]:
            func([workspace], fig, color_norm=creation_arg["norm"], normalize_by_bin_width=creation_arg["normalize_by_bin_width"])
            self.color_bar_remade = True
        else:
            func(workspace, **creation_arg)

    def plot_func(self, axes, creation_arg):
        """
        Calls plotting functions that aren't associated with workspaces, such as axhline and axvline.
        :param axes: matplotlib.Axes; Axes to call the function on
        :param creation_arg: The functions' arguments when it was originally called.
        """
        function_to_call = creation_arg.pop("function")
        function_dict = {"axhline": axes.axhline, "axvline": axes.axvline}

        func = function_dict[function_to_call]
        func(*creation_arg["args"], **creation_arg["kwargs"])

    def restore_figure_data(self, fig, dic):
        self.restore_fig_properties(fig, dic["properties"])
        axes_list = dic["axes"]
        for index, ax in enumerate(fig.axes):
            try:
                self.restore_fig_axes(ax, axes_list[index])
            except IndexError as e:
                if not self.color_bar_remade:
                    raise IndexError(e)
            except KeyError:
                logger.notice("Not adding data to blank axis.")

    @staticmethod
    def restore_fig_properties(fig, dic):
        fig.set_figheight(dic["figHeight"])
        fig.set_figwidth(dic["figWidth"])
        fig.set_dpi(dic["dpi"])

    def restore_fig_axes(self, ax, dic):
        # Restore axis properties
        properties = dic["properties"]
        self.update_properties(ax, properties)

        # Set the titles
        if dic["xAxisTitle"] is not None:
            ax.set_xlabel(dic["xAxisTitle"])
        if dic["yAxisTitle"] is not None:
            ax.set_ylabel(dic["yAxisTitle"])
        if dic["title"] is not None:
            ax.set_title(dic["title"])

        # Update the lines
        line_list = dic["lines"]
        for line in line_list:
            self.update_lines(ax, line)

        # Update/set text
        if "texts" in dic:
            for text_ in dic["texts"]:
                self.create_text_from_dict(ax, text_)

        # Update artists that are text
        if "textFromArtists" in dic:
            for artist in dic["textFromArtists"]:
                self.create_text_from_dict(ax, artist)

        # Update Legend
        self.update_legend(ax, dic["legend"])

        # Update colorbar if present
        if self.color_bar_remade and dic["colorbar"]["exists"]:
            if len(ax.images) > 0:
                image = ax.images[0]
            elif len(ax.collections) > 0:
                image = ax.images[0]
            else:
                raise RuntimeError("self.color_bar_remade set to True whilst no colorbar found")

            self.update_colorbar_from_dict(image, dic["colorbar"])

    @staticmethod
    def create_text_from_dict(ax, dic):
        style_dic = dic["style"]
        ax.text(
            x=dic["position"][0],
            y=dic["position"][1],
            s=dic["text"],
            fontdict={
                "alpha": style_dic["alpha"],
                "color": style_dic["color"],
                "rotation": style_dic["rotation"],
                "fontsize": style_dic["textSize"],
                "zorder": style_dic["zOrder"],
                "usetex": dic["useTeX"],
                "horizontalalignment": style_dic["hAlign"],
                "verticalalignment": style_dic["vAlign"],
            },
        )

    @staticmethod
    def update_lines(ax, line_settings):
        # update current line setting with settings from file
        def update_line_setting(line_update_method, settings, setting):
            if setting in settings:
                line_update_method(settings[setting])

        # get current line and update settings
        current_line = ax.lines[line_settings["lineIndex"]]

        update_line_setting(current_line.set_label, line_settings, "label")
        update_line_setting(current_line.set_alpha, line_settings, "alpha")
        update_line_setting(current_line.set_color, line_settings, "color")
        update_line_setting(current_line.set_linestyle, line_settings, "lineStyle")
        update_line_setting(current_line.set_linewidth, line_settings, "lineWidth")

        marker_style = line_settings["markerStyle"]
        update_line_setting(current_line.set_markerfacecolor, marker_style, "faceColor")
        update_line_setting(current_line.set_markeredgecolor, marker_style, "edgeColor")
        update_line_setting(current_line.set_markeredgewidth, marker_style, "edgeWidth")
        update_line_setting(current_line.set_marker, marker_style, "markerType")
        update_line_setting(current_line.set_markersize, marker_style, "markerSize")
        update_line_setting(current_line.set_zorder, marker_style, "zOrder")

        errorbar_style = line_settings["errorbars"]
        if errorbar_style["exists"]:
            update_line_setting(current_line.set_dash_capstyle, errorbar_style, "dashCapStyle")
            update_line_setting(current_line.set_dash_joinstyle, errorbar_style, "dashJoinStyle")
            update_line_setting(current_line.set_solid_capstyle, errorbar_style, "solidCapStyle")
            update_line_setting(current_line.set_solid_joinstyle, errorbar_style, "solidJoinStyle")

    @staticmethod
    def update_legend(ax, legend):
        if not legend["exists"] and ax.get_legend():
            ax.get_legend().remove()
            return
        if legend["exists"]:
            LegendProperties.create_legend(legend, ax)

    def update_properties(self, ax, properties):
        # Support for additonal plot options accessible from general settings
        if "tickParams" in properties.keys():
            ax.xaxis.set_tick_params(which="major", **properties["tickParams"]["xaxis"]["major"])
            ax.xaxis.set_tick_params(which="minor", **properties["tickParams"]["xaxis"]["minor"])

            ax.yaxis.set_tick_params(which="major", **properties["tickParams"]["yaxis"]["major"])
            ax.yaxis.set_tick_params(which="minor", **properties["tickParams"]["yaxis"]["minor"])

        if properties["bounds"]:
            ax.set_position(properties["bounds"])
        ax.set_navigate(properties["dynamic"])
        ax.axison = properties["axisOn"]
        ax.set_frame_on(properties["frameOn"])
        ax.set_visible(properties["visible"])

        ax.set_xscale(properties["xAxisScale"])
        ax.set_yscale(properties["yAxisScale"])
        if "xAutoScale" in properties and properties["xAutoScale"]:
            ax.autoscale(True, axis="x")
        else:
            ax.set_xlim(properties["xLim"])
        if "yAutoScale" in properties and properties["yAutoScale"]:
            ax.autoscale(True, axis="y")
        else:
            ax.set_ylim(properties["yLim"])
        ax.show_minor_gridlines = properties["showMinorGrid"]
        ax.set_facecolor(properties["facecolor"])

        # Update X Axis
        if "xAxisProperties" in properties:
            self.update_axis(ax.xaxis, properties["xAxisProperties"])

        # Update Y Axis
        if "yAxisProperties" in properties:
            self.update_axis(ax.yaxis, properties["yAxisProperties"])

        if "spineWidths" in properties:
            for spine, width in properties["spineWidths"].items():
                ax.spines[spine].set_linewidth(width)

    def update_axis(self, axis_, properties):
        if "position" in properties.keys():
            # Support for older .mtdproj files that did not include additional
            # plot settings introduced in PR #30121
            if isinstance(axis_, matplotlib.axis.XAxis):
                if properties["position"] == "top":
                    axis_.tick_top()
                else:
                    axis_.tick_bottom()

            if isinstance(axis_, matplotlib.axis.YAxis):
                if properties["position"] == "right":
                    axis_.tick_right()
                else:
                    axis_.tick_left()

        labels = axis_.get_ticklabels()
        if properties["fontSize"] != "":
            for label in labels:
                label.set_fontsize(properties["fontSize"])

        axis_.set_visible(properties["visible"])

        # Set axis tick data
        self.update_axis_ticks(axis_, properties)

        # Set grid data
        self.update_grid_style(axis_, properties)

    @staticmethod
    def update_grid_style(axis_, properties):
        grid_dict = properties["gridStyle"]
        grid_lines = axis_.get_gridlines()
        if grid_dict["gridOn"]:
            which = "both" if grid_dict["minorGridOn"] else "major"
            axis_.axes.grid(True, axis=axis_.axis_name, which=which)
            for grid_line in grid_lines:
                grid_line.set_alpha(grid_dict["alpha"])
                grid_line.set_color(grid_dict["color"])

    @staticmethod
    def update_axis_ticks(axis_, properties):
        # Update Major and Minor Locator
        if properties["majorTickLocator"] == "FixedLocator":
            axis_.set_major_locator(ticker.FixedLocator(properties["majorTickLocatorValues"]))

        if properties["minorTickLocator"] == "FixedLocator":
            axis_.set_minor_locator(ticker.FixedLocator(properties["minorTickLocatorValues"]))
        elif properties["minorTickLocator"] == "AutoMinorLocator":
            axis_.set_minor_locator(ticker.AutoMinorLocator())

        # Update Major and Minor TickFormatter
        fmt = get_tick_format(TICK_FORMATTERS, properties["majorTickFormatter"], properties["majorTickFormat"])
        axis_.set_major_formatter(fmt)
        fmt = get_tick_format(TICK_FORMATTERS, properties["minorTickFormatter"], properties["minorTickFormat"])
        axis_.set_minor_formatter(fmt)

    @staticmethod
    def update_colorbar_from_dict(image, dic):
        # colorbar = image.colorbar
        image.set_clim(*sorted([dic["min"], dic["max"]]))
        image.set_label(dic["label"])
        image.set_cmap(colormaps[dic["cmap"]])
        image.set_interpolation(dic["interpolation"])
        # Try and make the cmap line up but sometimes it wont
        try:
            image.axes.set_cmap(colormaps[dic["cmap"]])
        except AttributeError as e:
            logger.debug("PlotsLoader - The Image accessed did not have an axes with the ability to set the cmap: " + str(e))

        # Redraw
        image.axes.figure.canvas.draw()
