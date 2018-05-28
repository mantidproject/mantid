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

from workbench.plotting.currentfigure import CurrentFigure

class PlotSelectorModel(object):
    """
    This is a model for the plot selector widget. Currently
    this avoids storing any data directly, instead relying
    on the CurrentFigure singleton.
    """

    def __init__(self, presenter):
        """
        Initialise a new instance of PlotSelectorModel
        :param presenter: A presenter controlling this model.
        """
        self.presenter = presenter
        self.plot_list = []
        self.filtered_plot_list = []

    def get_plot_list(self):
        return self.plot_list

    def update_plot_list(self):
        self.plot_list = []
        figures = CurrentFigure.get_all_fig_managers()
        for figure in figures:
            self.plot_list.append(figure.get_window_title())

    def make_plot_active(self, plot_name):
        CurrentFigure.bring_to_front_by_name(plot_name)

    def register_observer(self, observer):
        CurrentFigure.add_observer(observer)

    def close_plot(self, plot_name):
        figure_number_to_close = CurrentFigure.get_figure_number_from_name(plot_name)
        CurrentFigure.destroy(figure_number_to_close)

    def filter_list_by_string(self, filter_text):
        self.filtered_plot_list = []
        for plot_name in self.plot_list:
            if filter_text in plot_name:
                self.filtered_plot_list.append(plot_name)
        return self.filtered_plot_list
