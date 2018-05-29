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
from .widget import PlotSelectorWidget


class PlotSelectorPresenter(object):
    """
    Presents (controls) a plot selector view. This UI element allows the user
    to select and make active a plot.
    """
    def __init__(self, current_figure_class, view = None, model = None):
        # Create model and view, or accept mocked versions
        if view is None:
            self.view = PlotSelectorWidget(self)
        else:
            self.view = view
        if model is None:
            self.model = PlotSelectorModel(self, current_figure_class)
        else:
            self.model = model

        # Make sure the plot list is up to date
        self.update_plot_list()

    def get_widget(self):
        return self.view

    def update_plot_list(self):
        self.model.update_plot_list()
        filter_text = self.view.get_filter_text()
        if not filter_text:
            self.view.set_plot_list(self.model.plot_list)
        else:
            self.filter_list_by_string(filter_text)

    # ----------------------- Plot Selection ------------------------

    def list_double_clicked(self):
        plot_name = self.view.get_currently_selected_plot_name()
        self.make_plot_active(plot_name)

    def make_plot_active(self, plot_name):
        self.model.make_plot_active(plot_name)

    # ----------------------- Plot Filtering ------------------------

    def filter_text_changed(self):
        filter_text = self.view.get_filter_text()
        self.filter_list_by_string(filter_text)

    def filter_list_by_string(self, filter_text):
        if not filter_text:
            self.view.set_plot_list(self.model.plot_list)
        else:
            filtered_plot_list = []
            for plot_name in self.model.plot_list:
                if filter_text in plot_name:
                    filtered_plot_list.append(plot_name)
            self.view.set_plot_list(filtered_plot_list)

    # ------------------------ Plot Closing -------------------------

    def close_button_clicked(self):
        selected_plots = self.view.get_all_selected_plot_names()
        self.close_plots(selected_plots)

    def close_plots(self, list_of_plots):
        for plot_name in list_of_plots:
            self.close_plot(plot_name)

    def close_plot(self, plot_name):
        self.model.close_plot(plot_name)

