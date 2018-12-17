# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

from matplotlib import ticker, text, axis  # noqa
import matplotlib.colors
import matplotlib.axes

from mantid import logger
from mantid.api import AnalysisDataService as ADS
from mantid.plots.plotfunctions import plot
from mantid import plots  # noqa


class PlotsLoader(object):
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
                logger.warning("A plot was unable to be loaded from the save file")

    def make_fig(self, plot_dict):
        """
        This method currently only considers single matplotlib.axes.Axes based figures as that is the most common case
        :param plot_dict:
        :return:
        """
        import matplotlib.pyplot as plt
        # Grab creation arguments
        creation_args = plot_dict["creationArguments"]
        workspace_name = creation_args[0][0].pop('workspaces')
        workspace = ADS.retrieve(workspace_name)
        # Make initial plot
        fig, ax = plt.subplots(subplot_kw={'projection': 'mantid'})

        self.plot_func(workspace, ax, creation_args[0][0])

        # If an overplot is necessary plot onto the same figure
        self.plot_extra_lines(creation_args=creation_args, ax=ax)

        # Update the fig
        fig._label = plot_dict["label"]
        fig.canvas.set_window_title(plot_dict["label"])
        self.restore_figure_data(fig=fig, dic=plot_dict)
        fig.show()

    @staticmethod
    def plot_func( workspace, axes, creation_arg):
        plot(workspace=workspace, axes=axes, **creation_arg)

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
                self.plot_func(workspace, ax, creation_args[0][ii])

    def restore_figure_data(self, fig, dic):
        self.restore_fig_properties(fig, dic["properties"])
        axes_list = dic["axes"]
        for index, ax in enumerate(fig.axes):
            self.restore_fig_axes(ax, axes_list[index])

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
        for index, text_ in enumerate(text_list):
            try:
                ax.texts[index] = self.create_text_from_dict(text_)
            except IndexError:
                ax.text(self.create_text_from_dict(text_))

        # Update artists that are text
        for artist in dic["textFromArtists"]:
            ax.text(self.create_text_from_dict(artist))

        # Update Legend
        legend = ax.get_legend()
        if legend is not None:
            self.update_legend(ax, dic["legend"])
        else:
            ax.legend()
            self.update_legend(ax, dic["legend"])

    @staticmethod
    def create_text_from_dict(dic):
        if dic["text"]:
            # If the text has a $\\ then it is assumed that it is meant to only be one back slash so one needs to be
            # removed
            style_dic = dic["style"]
            return matplotlib.text.Text(text='d-Spacing ()',
                                        # transform=dic["transform"],
                                        position=dic["position"],
                                        usetex=dic["useTeX"],
                                        alpha=style_dic["alpha"],
                                        fontsize=style_dic["textSize"],
                                        color=style_dic["color"],
                                        horizontalalignment=style_dic["hAlign"],
                                        verticalalignment=style_dic["vAlign"],
                                        multialignment=style_dic["mAlign"],
                                        rotation=style_dic["rotation"],
                                        zorder=style_dic["zOrder"])
        return None

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

    @staticmethod
    def update_legend(ax, legend):
        if not legend["exists"]:
            ax.get_legend().remove()
            return
        ax.legend().set_visible(legend["visible"])

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
