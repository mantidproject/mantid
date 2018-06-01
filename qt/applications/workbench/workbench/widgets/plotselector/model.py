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

    def update_plot_list(self):
        """
        Update the list of plots that is stored in this class, by
        getting the list from the GlobalFigureManager
        """
        self.plot_list = []
        figures = self.GlobalFigureManager.get_all_fig_managers()
        for figure in figures:
            self.plot_list.append(figure.get_window_title())

    def notify(self):
        """
        This is called by GlobalFigureManager when plots are created
        or destroyed. This calls the presenter to update the plot
        list in the model and the view.
        :return:
        """
        self.presenter.update_plot_list()

    def make_plot_active(self, plot_name):
        """
        For a given plot name make this plot active - bring it to the
        front and make it the destination for overplotting
        :param plot_name: A string with the name of the plot
                          (figure title)
        """
        figure_manager = self.GlobalFigureManager.get_figure_manager_from_name(plot_name)
        if figure_manager is not None:
            figure_manager.show()

    def close_plot(self, plot_name):
        """
        For a given plot close and remove all reference in the
        GlobalFigureManager
        :param plot_name: A string with the name of the plot
                          (figure title)
        """
        figure_number_to_close = self.GlobalFigureManager.get_figure_number_from_name(plot_name)
        if figure_number_to_close is not None:
            self.GlobalFigureManager.destroy(figure_number_to_close)
