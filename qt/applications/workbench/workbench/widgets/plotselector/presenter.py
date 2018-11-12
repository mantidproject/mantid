# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import absolute_import, print_function

import os
import re

from .model import PlotSelectorModel
from .view import PlotSelectorView, Column


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

    def get_plot_name_from_number(self, plot_number):
        return self.model.get_plot_name_from_number(plot_number)

    # ------------------------ Plot Updates ------------------------

    def update_plot_list(self):
        """
        Updates the plot list in the model and the view. Filter text
        is applied to the updated selection if required.
        """
        plot_list = self.model.get_plot_list()
        self.view.set_plot_list(plot_list)

    def append_to_plot_list(self, plot_number):
        """
        Appends the plot name to the end of the plot list
        :param plot_number: The unique number in GlobalFigureManager
        """
        self.view.append_to_plot_list(plot_number)
        self.view.set_visibility_icon(plot_number, self.model.is_visible(plot_number))

    def remove_from_plot_list(self, plot_number):
        """
        Removes the plot name from the plot list
        :param plot_number: The unique number in GlobalFigureManager
        """
        self.view.remove_from_plot_list(plot_number)

    def rename_in_plot_list(self, plot_number, new_name):
        """
        Replaces a name in the plot list
        :param plot_number: The unique number in GlobalFigureManager
        :param new_name: The new name for the plot
        """
        self.view.rename_in_plot_list(plot_number, new_name)

    # ----------------------- Plot Filtering ------------------------

    def filter_text_changed(self):
        """
        Called by the view when the filter text is changed (e.g. by
        typing or clearing the text)
        """
        if self.view.get_filter_text():
            self.view.filter_plot_list()
        else:
            self.view.unhide_all_plots()

    def is_shown_by_filter(self, plot_number):
        """
        :param plot_number: The unique number in GlobalFigureManager
        :return: True if shown, or False if filtered out
        """
        filter_text = self.view.get_filter_text()
        plot_name = self.get_plot_name_from_number(plot_number)
        return filter_text.lower() in plot_name.lower()

    # ------------------------ Plot Showing ------------------------

    def show_single_selected(self):
        """
        When a list item is double clicked the view calls this method
        to bring the selected plot to the front
        """
        plot_number = self.view.get_currently_selected_plot_number()
        self._make_plot_active(plot_number)

    def show_multiple_selected(self):
        """
        Shows multiple selected plots, e.g. from pressing the 'Show'
        button with multiple selected plots
        """
        selected_plots = self.view.get_all_selected_plot_numbers()
        for plot_number in selected_plots:
            self._make_plot_active(plot_number)

    def _make_plot_active(self, plot_number):
        """
        Make the plot with the given name active - bring it to the
        front and make it the choice for overplotting
        :param plot_number: The unique number in GlobalFigureManager
        """
        try:
            self.model.show_plot(plot_number)
        except ValueError as e:
            print(e)

    def set_active_font(self, plot_number):
        """
        Set the icon for the active plot to be colored
        :param plot_number: The unique number in GlobalFigureManager
        """
        active_plot_number = self.view.active_plot_number
        if active_plot_number > 0:
            try:
                self.view.set_active_font(active_plot_number, False)
            except TypeError:
                pass
                # The last active plot could have been closed
                # already, so there is nothing to do
        self.view.set_active_font(plot_number, True)
        self.view.active_plot_number = plot_number

    # ------------------------ Plot Hiding -------------------------

    def hide_selected_plots(self):
        """
        Hide all plots that are selected in the view
        """
        selected_plots = self.view.get_all_selected_plot_numbers()

        for plot_number in selected_plots:
            self._hide_plot(plot_number)

    def _hide_plot(self, plot_number):
        """
        Hides a single plot
        """
        try:
            self.model.hide_plot(plot_number)
        except ValueError as e:
            print(e)

    def toggle_plot_visibility(self, plot_number):
        """
        Toggles a plot between hidden and shown
        :param plot_number: The unique number in GlobalFigureManager
        """
        if self.model.is_visible(plot_number):
            self._hide_plot(plot_number)
        else:
            self._make_plot_active(plot_number)

        self.update_visibility_icon(plot_number)

    def update_visibility_icon(self, plot_number):
        """
        Updates the icon to indicate a plot as hidden or visible
        :param plot_number: The unique number in GlobalFigureManager
        """
        try:
            is_visible = self.model.is_visible(plot_number)
            self.view.set_visibility_icon(plot_number, is_visible)
        except ValueError:
            # There is a chance the plot was closed, which calls an
            # update to this method. If we can not get the visibility
            # status it is safe to assume the plot has been closed.
            pass

    # ------------------------ Plot Renaming ------------------------

    def rename_figure(self, plot_number, new_name):
        """
        Replaces a name in the plot list
        :param plot_number: The unique number in GlobalFigureManager
        :param new_name: The new plot name
         """
        try:
            self.model.rename_figure(plot_number, new_name)
        except ValueError as e:
            # We need to undo the rename in the view
            self.view.rename_in_plot_list(plot_number, new_name)
            print(e)

    # ------------------------ Plot Closing -------------------------

    def close_action_called(self):
        """
        This is called by the view when closing plots is requested
        (e.g. pressing close or delete).
        """
        selected_plots = self.view.get_all_selected_plot_numbers()
        self._close_plots(selected_plots)

    def close_single_plot(self, plot_number):
        """
        This is used to close plots when a close action is called
        that does not refer to the selected plot(s)
        :param plot_number: The unique number in GlobalFigureManager
        """
        self._close_plots([plot_number])

    def _close_plots(self, list_of_plot_numbers):
        """
        Accepts a list of plot names to close
        :param list_of_plots: A list of strings containing plot names
        """
        for plot_number in list_of_plot_numbers:
            try:
                self.model.close_plot(plot_number)
            except ValueError as e:
                print(e)

    # ----------------------- Plot Sorting --------------------------

    def set_sort_order(self, is_ascending):
        """
        Sets the sort order in the view
        :param is_ascending: If true ascending order, else descending
        """
        self.view.set_sort_order(is_ascending)

    def set_sort_type(self, sort_type):
        """
        Sets the sort order in the view
        :param sort_type: A Column enum with the column to sort on
        """
        self.view.set_sort_type(sort_type)
        self.update_last_active_order()

    def update_last_active_order(self):
        """
        Update the sort keys in the view. This is only required when
        changes to the last shown order occur in the model, when
        renaming the key is set already
        """
        if self.view.sort_type() == Column.LastActive:
            self._set_last_active_order()

    def _set_last_active_order(self):
        """
        Set the last shown order in the view. This checks the sorting
        currently set and then sets the sort keys to the appropriate
        values
        """
        last_active_values = self.model.last_active_values()
        self.view.set_last_active_values(last_active_values)

    def get_initial_last_active_value(self, plot_number):
        """
        Gets the initial last active value for a plot just added, in
        this case it is assumed to not have been shown
        :param plot_number: The unique number in GlobalFigureManager
        :return: A string with the last active value
        """
        return '_' + self.model.get_plot_name_from_number(plot_number)

    def get_renamed_last_active_value(self, plot_number, old_last_active_value):
        """
        Gets the initial last active value for a plot that was
        renamed. If the plot had a numeric value, i.e. has been shown
        this is retained, else it is set
        :param plot_number: The unique number in GlobalFigureManager
        :param old_last_active_value: The previous last active value
        """
        if old_last_active_value.isdigit():
            return old_last_active_value
        else:
            return self.get_initial_last_active_value(plot_number)

    # ---------------------- Plot Exporting -------------------------

    def export_plots_called(self, extension):
        """
        Export plots called from the view, then a single or multiple
        plots exported depending on the number currently selected
        :param extension: The file extension as a string including
                          a '.', for example '.png' (must be a type
                          supported by matplotlib)
        """
        plot_numbers = self.view.get_all_selected_plot_numbers()

        if len(plot_numbers) == 1:
            self._export_single_plot(plot_numbers[0], extension)
        elif len(plot_numbers) > 1:
            self._export_multiple_plots(plot_numbers, extension)

    def _export_single_plot(self, plot_number, extension):
        """
        Called when a single plot is selected to export - prompts for
        a filename then tries to save the plot
        :param plot_number: The unique number in GlobalFigureManager
        :param extension: The file extension as a string including
                          a '.', for example '.png' (must be a type
                          supported by matplotlib)
        """
        absolute_path = self.view.get_file_name_for_saving(extension)

        if not absolute_path[-4:] == extension:
            absolute_path += extension

        try:
            self.model.export_plot(plot_number, absolute_path)
        except ValueError as e:
            print(e)

    def _export_multiple_plots(self, plot_numbers, extension):
        """
        Export all selected plots in the plot_numbers list, first
        prompting for a save directory then sanitising plot names to
        unique, usable file names
        :param plot_numbers: A list of plot numbers to export
        :param extension: The file extension as a string including
                          a '.', for example '.png' (must be a type
                          supported by matplotlib)
        """
        dir_name = self.view.get_directory_name_for_saving()

        # A temporary dictionary holding plot numbers as keys, plot
        # names as values
        plots = {}

        for plot_number in plot_numbers:
            plot_name = self.model.get_plot_name_from_number(plot_number)
            plot_name = self._replace_special_characters(plot_name)
            if plot_name in plots.values():
                plot_name = self._make_unique_name(plot_name, plots)
            plots[plot_number] = plot_name

            self._export_plot(plot_number, plot_name, dir_name, extension)

    def _replace_special_characters(self, string):
        """
        Removes any characters that are not valid in file names
        across all operating systems ('/' for Linux/Mac), more for
        Windows
        :param string: The string to replace characters in
        :return: The string with special characters replace by '-'
        """
        return re.sub(r'[<>:"/|\\?*]', r'-', string)

    def _make_unique_name(self, name, dictionary):
        """
        Given a name and a dictionary, make a unique name that does
        not already exist in the dictionary values by appending
        ' (1)', ' (2)', ' (3)' etc. to the end of the name
        :param name: A string with the non-unique name
        :param dictionary: A dictionary with string values
        :return : The unique plot name
        """
        i = 1
        while True:
            plot_name_attempt = name + ' ({})'.format(str(i))
            if plot_name_attempt not in dictionary.values():
                break
            i += 1

        return plot_name_attempt

    def _export_plot(self, plot_number, plot_name, dir_name, extension):
        """
        Given a plot number, plot name, directory and extension
        construct the absolute path name and call the model to save
        the figure
        :param plot_number: The unique number in GlobalFigureManager
        :param plot_name: The name to use for saving
        :param dir_name: The directory to save to
        :param extension: The file extension as a string including
                          a '.', for example '.png' (must be a type
                          supported by matplotlib)
        """
        if dir_name:
            filename = os.path.join(dir_name, plot_name + extension)
            try:
                self.model.export_plot(plot_number, filename)
            except ValueError as e:
                print(e)
