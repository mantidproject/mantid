#  This file is part of the mantid workbench.
#
#  Copyright (C) 2018 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import absolute_import, print_function

from workbench.plotting.globalfiguremanager import FigureAction


class PlotSelectorModel(object):
    """
    This is the model for the plot selector widget. Essentially this
    is just a wrapper to the true model - GlobalFigureManager.
    """
    def __init__(self, presenter, global_figure_manager):
        """
        Initialise the model, keeping references to the presenter and
        GlobalFigureManager
        :param presenter: The presenter controlling this model
        :param global_figure_manager: The GlobalFigureManager
                                      singleton controlling plotting
        """
        self.GlobalFigureManager = global_figure_manager
        self.presenter = presenter

        # Register with CurrentFigure that we want to know of any
        # changes to the list of plots
        self.GlobalFigureManager.add_observer(self)

    def get_plot_name_from_number(self, plot_number):
        figure_manager = self.GlobalFigureManager.figs.get(plot_number)
        if figure_manager is None:
            return ''
        else:
            return figure_manager.get_window_title()

    # ------------------------ Plot Updates ------------------------

    def get_plot_list(self):
        """
        Returns a dictionary with the list of plots in the
        GlobalFigureManager, with figure number as the keys and the
        plot name as the values
        :return: A dictionary with figure numbers as keys, and plot
                 names as values
        """
        plot_dict = {}
        figures = self.GlobalFigureManager.get_all_fig_managers()
        for figure in figures:
            self.plot_dict[figure.num] = figure.get_window_title()
        return plot_dict

    def notify(self, action, plot_number):
        """
        This is called by GlobalFigureManager when plots are created
        or destroyed, renamed or the active order is changed. This
        calls the presenter to update the plot list in the model and
        the view.
        :param action: A FigureAction corresponding to the event
        :param plot_number: The unique number in GlobalFigureManager
        """
        if action == FigureAction.New:
            self.presenter.append_to_plot_list(plot_number)
        if action == FigureAction.Closed:
            self.presenter.remove_from_plot_list(plot_number)
        if action == FigureAction.Renamed:
            figure_manager = self.GlobalFigureManager.figs.get(plot_number)
            # This can be triggered before the plot is added to the
            # GlobalFigureManager, so we silently ignore this case
            if figure_manager is not None:
                self.presenter.rename_in_plot_list(plot_number, figure_manager.get_window_title())
        if action == FigureAction.OrderChanged:
            self.presenter.update_last_active_order()
        if action == FigureAction.Unknown:
            self.presenter.update_plot_list()

    # ------------------------ Plot Showing ------------------------

    def show_plot(self, plot_number):
        """
        For a given plot name make this plot active - bring it to the
        front and make it the destination for overplotting
        :param plot_number: The unique number in GlobalFigureManager
        """
        figure_manager = self.GlobalFigureManager.figs.get(plot_number)
        if figure_manager is None:
            raise ValueError('Error showing, could not find a plot with the number {}.'.format(plot_number))
        figure_manager.show()

    # ------------------------ Plot Renaming ------------------------

    def rename_figure(self, plot_number, new_name):
        """
        Renames a figure in the GlobalFigureManager
        :param plot_number: The unique number in GlobalFigureManager
        :param new_name: The new figure (plot) name
        """
        figure_manager = self.GlobalFigureManager.figs.get(plot_number)
        if figure_manager is None:
            raise ValueError('Error renaming, could not find a plot with the number {}.'.format(plot_number))

        figure_manager.set_window_title(new_name)

    # ------------------------ Plot Closing -------------------------

    def close_plot(self, plot_number):
        """
        For a given plot close and remove all reference in the
        GlobalFigureManager
        :param plot_number: The unique number in GlobalFigureManager
        """
        figure_manager = self.GlobalFigureManager.figs.get(plot_number)
        if figure_manager is None:
            raise ValueError('Error closing, could not find a plot with the number {}.'.format(plot_number))

        self.GlobalFigureManager.destroy(plot_number)

    # ----------------------- Plot Sorting --------------------------

    def last_active_values(self):
        """
        Returns a dictionary containing the order of the last shown
        plots. Not all plots are guaranteed to be in returned
        dictionary.
        :return: A dictionary containing the plot numbers as keys,
                 and the order (1...N) as values
                 (e.g. {1: 2, 2: 1, 7: 3})
        """
        return self.GlobalFigureManager.last_active_values()

    # ---------------------- Plot Exporting -------------------------

    def export_plot(self, plot_number, save_absolute_path):
        """
        Export a plot, with the type based on the the filename
        extension
        :param plot_number: The unique number in GlobalFigureManager
        :param save_absolute_path: The absolute path, with the
                                   extension giving the type
        :return:
        """
        figure_manager = self.GlobalFigureManager.figs.get(plot_number)
        if figure_manager is None:
            raise ValueError('Error closing, could not find a plot with the number {}.'.format(plot_number))

        try:
            figure_manager.canvas.figure.savefig(save_absolute_path)
        except IOError:
            # TODO: Fix this error message
            raise ValueError("Error, could not save plot with name {} because the filename is invalid. "
                             "Please remove any characters in the plot name that cannot be used in filenames.")
