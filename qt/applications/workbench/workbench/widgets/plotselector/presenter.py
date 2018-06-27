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

import os

from .model import PlotSelectorModel
from .view import PlotSelectorView, SortType


class PlotSelectorPresenter(object):
    """
    Presenter for the plot selector widget. This class can be
    responsible for the creation of the model and view, passing in
    the GlobalFigureManager as an argument, or the presenter and view
    can be passed as arguments (only intended for testing).
    """
    def __init__(self, global_figure_manager, view=None, model=None):
        """
        Initialise the presenter, creating the view and model, and
        setting the initial plot list
        :param global_figure_manager: The GlobalFigureManager class
        :param view: Optional - a view to use instead of letting the
                     class create one (intended for testing)
        :param model: Optional - a model to use instead of letting
                      the class create one (intended for testing)
        """
        # Create model and view, or accept mocked versions
        if view is None:
            self.view = PlotSelectorView(self)
        else:
            self.view = view
        if model is None:
            self.model = PlotSelectorModel(self, global_figure_manager)
        else:
            self.model = model

        # Make sure the plot list is up to date
        self.update_plot_list()

    # ------------------------ Plot Updates ------------------------

    def update_plot_list(self):
        """
        Updates the plot list in the model and the view. Filter text
        is applied to the updated selection if required.
        """
        self.model.update_plot_list()
        filter_text = self.view.get_filter_text()
        if not filter_text:
            self.view.set_plot_list(self.model.plot_list)
        else:
            self._filter_plot_list_by_string(filter_text)

    def append_to_plot_list(self, plot_name):
        """
        Appends the plot name to the end of the plot list
        :param plot_name: The name of the plot
        """
        try:
            self.model.append_to_plot_list(plot_name)
            self.view.append_to_plot_list(plot_name)
        except ValueError as e:
            print(e)

    def remove_from_plot_list(self, plot_name):
        """
        Removes the plot name from the plot list
        :param plot_name: The name of the plot
        """
        try:
            self.model.remove_from_plot_list(plot_name)
            self.view.remove_from_plot_list(plot_name)
        except ValueError as e:
            print(e)

    def rename_in_plot_list(self, new_name, old_name):
        """
        Replaces a name in the plot list
        :param new_name: The new plot name
        :param old_name: The name of the plot to be replaced
        """
        if new_name == old_name:
            return

        try:
            self.model.rename_in_plot_list(new_name, old_name)
            self.view.rename_in_plot_list(new_name, old_name)
        except ValueError as e:
            print(e)

    # ----------------------- Plot Filtering ------------------------

    def filter_text_changed(self):
        """
        Called by the view when the filter text is changed (e.g. by
        typing or clearing the text)
        """
        filter_text = self.view.get_filter_text()
        self._filter_plot_list_by_string(filter_text)

    def _filter_plot_list_by_string(self, filter_text):
        """
        Given a string to filter on this updates the list of plots
        in the view or shows all plots if the string is empty
        :param filter_text: A string containing the filter text
        """
        if not filter_text:
            self.view.set_plot_list(self.model.plot_list)
        else:
            filtered_plot_list = []
            for plot_name in self.model.plot_list:
                if filter_text.lower() in plot_name.lower():
                    filtered_plot_list.append(plot_name)
            self.view.set_plot_list(filtered_plot_list)

    # ------------------------ Plot Showing ------------------------

    def show_single_selected(self):
        """
        When a list item is double clicked the view calls this method
        to bring the selected plot to the front
        """
        plot_name = self.view.get_currently_selected_plot_name()
        self.make_plot_active(plot_name)

    def show_multiple_selected(self):
        selected_plots = self.view.get_all_selected_plot_names()
        for plot_name in selected_plots:
            self.make_plot_active(plot_name)

    def make_plot_active(self, plot_name):
        """
        Make the plot with the given name active - bring it to the
        front and make it the choice for overplotting
        :param plot_name: The name of the plot
        """
        try:
            self.model.show_plot(plot_name)
        except ValueError as e:
            print(e)

    # ------------------------ Plot Renaming ------------------------

    def rename_figure(self, new_name, old_name):
        """
        Replaces a name in the plot list
        :param new_name: The new plot name
        :param old_name: The name of the plot to be replaced
        """
        if new_name == old_name:
            return

        try:
            self.model.rename_figure(new_name, old_name)
        except ValueError as e:
            # We need to undo the rename in the view
            self.view.rename_in_plot_list(old_name, old_name)
            print(e)

    # ------------------------ Plot Closing -------------------------

    def close_action_called(self):
        """
        This is called by the view when closing plots is requested
        (e.g. pressing close or delete).
        """
        selected_plots = self.view.get_all_selected_plot_names()
        self._close_plots(selected_plots)

    def close_single_plot(self, plot_name):
        """
        This is used to close plots when a close action is called
        that does not refer to the selected plot(s)
        :param plot_name: Name of the plot to close
        """
        self._close_plots([plot_name])

    def _close_plots(self, list_of_plots):
        """
        Accepts a list of plot names to close
        :param list_of_plots: A list of strings containing plot names
        """
        for plot_name in list_of_plots:
            try:
                self.model.close_plot(plot_name)
            except ValueError as e:
                print(e)

    # ----------------------- Plot Sorting --------------------------

    def set_sort_order(self, is_ascending):
        if is_ascending:
            self.view.sort_ascending()
        else:
            self.view.sort_descending()

    def set_sort_type(self, is_by_name):
        if is_by_name:
            self.view.sort_by_name()
        else:
            self.view.sort_by_last_shown()
        self._set_sort_keys()

    def update_sort_keys(self):
        if self.view.sort_type == SortType.LastShown:
            self._set_sort_keys()

    def _set_sort_keys(self):
        sort_keys = {}

        if self.view.sort_type == SortType.Name:
            sort_keys = dict(zip(self.model.plot_list, self.model.plot_list))
        elif self.view.sort_type == SortType.LastShown:
            sort_keys = self.model.last_shown_order_dict()
            for plot_name in self.model.plot_list:
                if plot_name not in sort_keys:
                    # Append an '_' to the plot name - it has never been
                    # shown so goes after the numbers
                    sort_keys[plot_name] = '_' + plot_name

        try:
            self.view.set_sort_keys(sort_keys)
        except KeyError as e:
            print('Error, plot list out of sync, reloading. Error was:')
            print(e)
            self.update_plot_list()

    def get_initial_sort_key(self, plot_name):
        if self.view.sort_type == SortType.LastShown:
            return '_' + plot_name
        return plot_name

    # ---------------------- Plot Exporting -------------------------

    def export_plots(self, path, extension):
        for plot_name in self.view.get_all_selected_plot_names():
            self.export_plot(plot_name, path, extension)

    def export_plot(self, plot_name, path, extension):
        if path:
            filename = os.path.join(path, plot_name + extension)
            try:
                self.model.export_plot(plot_name, filename)
            except ValueError as e:
                print(e)
