# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

import copy

from matplotlib import ticker, text, axis  # noqa
import matplotlib.colors
import matplotlib.axes
import matplotlib.cm as cm

from mantid import logger
from mantid.api import AnalysisDataService as ADS
from mantid import plots  # noqa
from mantidqt.plotting.functions import pcolormesh

# Constants set in workbench.plotting.functions but would cause backwards reliability
SUBPLOT_WSPACE = 0.5
SUBPLOT_HSPACE = 0.5


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

    def make_fig(self, plot_dict, create_plot=True):
        """
        This method currently only considers single matplotlib.axes.Axes based figures as that is the most common case
        :param plot_dict: dictionary; A dictionary of various items intended to recreate a figure
        :param create_plot: Bool; whether or not to make the plot, or to return the figure.
        :return: matplotlib.figure; Only returns if create_plot=False
        """
        import matplotlib.pyplot as plt
        # Grab creation arguments
        creation_args = plot_dict["creationArguments"]

        # Make a copy so it can be applied to the axes, of the plot once created.
        creation_args_copy = copy.deepcopy(creation_args[0])

        # Populate workspace names
        workspace_name = creation_args[0][0].pop('workspaces')
        workspace = ADS.retrieve(workspace_name)

        # Make initial plot
        fig, ax = plt.subplots(subplot_kw={'projection': 'mantid'})

        # Make sure that the axes gets it's creation_args as loading doesn't add them
        ax.creation_args = creation_args_copy

        self.plot_func(workspace, ax, fig, creation_args[0][0])

        # If an overplot is necessary plot onto the same figure
        self.plot_extra_lines(creation_args=creation_args, ax=ax)

        # Update the fig
        fig._label = plot_dict["label"]
        fig.canvas.set_window_title(plot_dict["label"])
        self.restore_figure_data(fig=fig, dic=plot_dict)

        # If the function should create plot then create else return
        if create_plot:
            fig.show()
        else:
            return fig

    def plot_func(self, workspace, axes, fig,creation_arg):
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

        function_dict = {"plot": axes.plot, "scatter": axes.scatter, "errorbar": axes.errorbar,
                         "pcolor": axes.pcolor, "pcolorfast": axes.pcolorfast,"pcolormesh": pcolormesh,
                         "imshow": pcolormesh, "contour": axes.contour, "contourf": axes.contourf,
                         "tripcolor": axes.tripcolor, "tricontour": axes.tricontour, "tricontourf": axes.tricontourf}

        func = function_dict[function_to_call]
        # Plotting is done via an Axes object unless a colorbar needs to be added
        if function_to_call in ["imshow", "pcolormesh"]:
            func([workspace], fig)
            self.color_bar_remade = True
        else:
            func(workspace, **creation_arg)

    def plot_extra_lines(self, creation_args, ax):
        """
        This method currently only considers single matplotlib.axes.Axes based figures as that is the most common case,
        to make it more than that the lines creation_args[0] needs to be rewrote to handle multiple axes args.
        :param creation_args:
        :param ax:
        :return:
        """
        # If an overplot is necessary plot onto the same figure
        if len(creation_args[0]) > 1:
            for ii in range(1, len(creation_args[0])):
                workspace_name = creation_args[0][ii].pop('workspaces')
                workspace = ADS.retrieve(workspace_name)
                self.plot_func(workspace, ax, ax.figure, creation_args[0][ii])

    def restore_figure_data(self, fig, dic):
        self.restore_fig_properties(fig, dic["properties"])
        axes_list = dic["axes"]
        for index, ax in enumerate(fig.axes):
            try:
                self.restore_fig_axes(ax, axes_list[index])
            except IndexError as e:
                if not self.color_bar_remade:
                    raise IndexError(e)

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
        text_list = dic["texts"]
        for text_ in text_list:
            self.create_text_from_dict(ax, text_)

        # Update artists that are text
        for artist in dic["textFromArtists"]:
            self.create_text_from_dict(ax, artist)

        # Update Legend
        legend = ax.get_legend()
        if legend is not None:
            self.update_legend(ax, dic["legend"])
        else:
            ax.legend()
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
        ax.text(x=dic["position"][0],
                y=dic["position"][1],
                s=dic["text"],
                fontdict={u'alpha': style_dic["alpha"],
                          u'color': style_dic["color"],
                          u'rotation': style_dic["rotation"],
                          u'fontsize': style_dic["textSize"],
                          u'zorder': style_dic["zOrder"],
                          u'usetex': dic["useTeX"],
                          u'horizontalalignment': style_dic["hAlign"],
                          u'verticalalignment': style_dic["vAlign"]})

    @staticmethod
    def update_lines(ax, line):
        # Find the line based on label
        current_line = ax.lines[line["lineIndex"]]

        current_line.set_label(line["label"])
        current_line.set_alpha(line["alpha"])
        current_line.set_color(line["color"])
        current_line.set_linestyle(line["lineStyle"])
        current_line.set_linewidth(line["lineWidth"])

        marker_style = line["markerStyle"]
        current_line.set_markerfacecolor(marker_style["faceColor"])
        current_line.set_markeredgecolor(marker_style["edgeColor"])
        current_line.set_markeredgewidth(marker_style["edgeWidth"])
        current_line.set_marker(marker_style["markerType"])
        current_line.set_markersize(marker_style["markerSize"])
        current_line.set_zorder(marker_style["zOrder"])

        errorbar_style = line["errorbars"]
        if errorbar_style["exists"]:
            current_line.set_dash_capstyle(errorbar_style["dashCapStyle"])
            current_line.set_dash_joinstyle(errorbar_style["dashJoinStyle"])
            current_line.set_solid_capstyle(errorbar_style["solidCapStyle"])
            current_line.set_solid_joinstyle(errorbar_style["solidJoinStyle"])

    @staticmethod
    def update_legend(ax, legend):
        if not legend["exists"]:
            ax.get_legend().remove()
            return
        ax.legend().set_visible(legend["visible"])

        # Ensure that legend is draggable
        ax.get_legend().draggable()

    def update_properties(self, ax, properties):
        ax.set_position(properties["bounds"])
        ax.set_navigate(properties["dynamic"])
        ax.axison = properties["axisOn"]
        ax.set_frame_on(properties["frameOn"])
        ax.set_visible(properties["visible"])

        # Update X Axis
        self.update_axis(ax.xaxis, properties["xAxisProperties"])

        # Update Y Axis
        self.update_axis(ax.yaxis, properties["yAxisProperties"])

        ax.set_xscale(properties["xAxisScale"])
        ax.set_yscale(properties["yAxisScale"])
        ax.set_xlim(properties["xLim"])
        ax.set_ylim(properties["yLim"])

    def update_axis(self, axis_, properties):
        if isinstance(axis_, matplotlib.axis.XAxis):
            if properties["position"] is "top":
                axis_.tick_top()
            else:
                axis_.tick_bottom()

        if isinstance(axis_, matplotlib.axis.YAxis):
            if properties["position"] is "right":
                axis_.tick_right()
            else:
                axis_.tick_left()

        labels = axis_.get_ticklabels()
        if properties["fontSize"] is not "":
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
            axis_._gridOnMajor = True
            for grid_line in grid_lines:
                grid_line.set_alpha(grid_dict["alpha"])
                grid_line.set_color(grid_dict["color"])

    @staticmethod
    def update_axis_ticks(axis_, properties):
        # Update Major and Minor Locator
        if properties["majorTickLocator"] is "FixedLocator":
            axis_.set_major_locator(ticker.FixedLocator(properties["majorTickLocatorValues"]))

        if properties["minorTickLocator"] is "FixedLocator":
            axis_.set_minor_locator(ticker.FixedLocator(properties["minorTickLocatorValues"]))

        # Update Major and Minor Formatter
        if properties["majorTickFormatter"] is "FixedFormatter":
            axis_.set_major_formatter(ticker.FixedFormatter(properties["majorTickFormat"]))

        if properties["minorTickFormatter"] is "FixedFormatter":
            axis_.set_major_formatter(ticker.FixedLocator(properties["minorTickFormat"]))

    @staticmethod
    def update_colorbar_from_dict(image, dic):
        # colorbar = image.colorbar
        image.set_clim(*sorted([dic["min"], dic["max"]]))
        image.set_label(dic["label"])
        image.set_cmap(cm.get_cmap(dic["cmap"]))
        image.set_interpolation(dic["interpolation"])
        #Try and make the cmap line up but sometimes it wont
        try:
            image.axes.set_cmap(cm.get_cmap(dic["cmap"]))
        except AttributeError as e:
            logger.debug("PlotsLoader - The Image accessed did not have an axes with the ability to set the cmap: "
                         + str(e))

        # Redraw
        image.axes.figure.canvas.draw()
