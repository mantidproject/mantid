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

from .model import PlotSelectorModel


class PlotSelectorPresenter(object):
    """
    Presents (controls) a plot selector view. This UI element allows the user
    to select and make active a plot.
    """
    def __init__(self, view):
        self.view = view
        self.model = PlotSelectorModel(self)
        self.view.init_ui()
        plot_list = self.model.get_plot_list()
        self.view.set_plot_list(plot_list)
        self.model.register_observer(self)

    def update_plot_list(self):
        self.model.update_plot_list()
        filter_text = self.view.get_filter_string()
        if not filter_text:
            self.view.set_plot_list(self.model.plot_list)
        else:
            filtered_plot_list = self.model.filter_list_by_string(filter_text)
            self.view.set_plot_list(filtered_plot_list)

    def make_plot_active(self, plot_name):
        self.model.make_plot_active(plot_name)

    def notify(self):
        self.update_plot_list()

    def close_plot(self, plot_name):
        self.model.close_plot(plot_name)

    def close_plots(self, list_of_plots):
        for plot_name in list_of_plots:
            self.close_plot(plot_name)

    def filter_list_by_string(self, filter_text):
        if not filter_text:
            self.view.set_plot_list(self.model.get_plot_list())
        else:
            filtered_plot_list = self.model.filter_list_by_string(filter_text)
            self.view.set_plot_list(filtered_plot_list)

