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
        self.plot_list = []

        # Register with CurrentFigure that we want to know of any
        # changes to the list of plots
        self.GlobalFigureManager.add_observer(self)

    # ------------------------ Plot Updates ------------------------

    def update_plot_list(self):
        """
        Update the list of plots that is stored in this class, by
        getting the list from the GlobalFigureManager
        """
        self.plot_list = []
        figures = self.GlobalFigureManager.get_all_fig_managers()
        for figure in figures:
            self.plot_list.append(figure.get_window_title())

    def append_to_plot_list(self, plot_name):
        if plot_name in self.plot_list:
            raise ValueError('Error appending, name {} already in use.'.format(plot_name))

        self.plot_list.append(plot_name)

    def remove_from_plot_list(self, plot_name):
        if plot_name not in self.plot_list:
            raise ValueError('Error removing, could not find a plot with the name {}.'.format(plot_name))

        self.plot_list.remove(plot_name)

    def rename_in_plot_list(self, new_name, old_name):
        if old_name not in self.plot_list:
            raise ValueError('Error renaming, could not find a plot with the name {}.'.format(old_name))

        if new_name in self.plot_list:
            raise ValueError('Error renaming, name {} already in use.'.format(new_name))

        if new_name == old_name:
            raise ValueError('Error renaming, old name and new name the same: {}.'.format(new_name))

        self.plot_list = [new_name if plot_name == old_name else plot_name for plot_name in self.plot_list]

    def notify(self, action, plot_name):
        """
        This is called by GlobalFigureManager when plots are created
        or destroyed. This calls the presenter to update the plot
        list in the model and the view.
        :return:
        """
        if action == FigureAction.New:
            self.presenter.append_to_plot_list(plot_name)
        if action == FigureAction.Closed:
            self.presenter.remove_from_plot_list(plot_name)
        if action == FigureAction.Renamed:
            new_name, old_name = plot_name
            self.presenter.rename_in_plot_list(new_name, old_name)
        if action == FigureAction.Unknown:
            self.presenter.update_plot_list()

    # ------------------------ Plot Showing ------------------------

    def show_plot(self, plot_name):
        """
        For a given plot name make this plot active - bring it to the
        front and make it the destination for overplotting
        :param plot_name: A string with the name of the plot
                          (figure title)
        """
        figure_manager = self.GlobalFigureManager.get_figure_manager_from_name(plot_name)
        if figure_manager is not None:
            figure_manager.show()
        else:
            raise ValueError('Error, could not find a plot with the name {}.'.format(plot_name))

    # ------------------------ Plot Renaming ------------------------

    def rename_figure(self, new_name, old_name):
        if old_name not in self.plot_list:
            raise ValueError('Error renaming, could not find a plot with the name {}.'.format(old_name))

        if new_name in self.plot_list:
            raise ValueError('Error renaming, name {} already in use.'.format(new_name))

        figure = self.GlobalFigureManager.get_figure_manager_from_name(old_name)
        figure.set_window_title(new_name)

    # ------------------------ Plot Closing -------------------------

    def close_plot(self, plot_name):
        """
        For a given plot close and remove all reference in the
        GlobalFigureManager
        :param plot_name: A string with the name of the plot
                          (figure title)
        """
        if plot_name not in self.plot_list:
            print(self.plot_list)
            raise ValueError('Error closing, could not find a plot with the name {}.'.format(plot_name))

        figure_number_to_close = self.GlobalFigureManager.get_figure_number_from_name(plot_name)
        if figure_number_to_close is not None:
            self.GlobalFigureManager.destroy(figure_number_to_close)

    # ---------------------- Plot Exporting -------------------------

    def export_plot(self, plot_name, save_path):
        if plot_name not in self.plot_list:
            raise ValueError('Error renaming, could not find a plot with the name {}.'.format(plot_name))

        figure_manager = self.GlobalFigureManager.get_figure_manager_from_name(plot_name)

        try:
            figure_manager.canvas.figure.savefig(save_path)
        except IOError as e:
            raise ValueError("Error, could not save plot with name {} because the filename is invalid. "
                             "Please remove any characters in the plot name that cannot be used in filenames."
                             .format(plot_name))
