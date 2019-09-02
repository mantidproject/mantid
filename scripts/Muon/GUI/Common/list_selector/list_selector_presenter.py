# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, unicode_literals)

model_position = 0
model_selected = 1
model_enabled = 2

view_item_name = 0
view_item_selected = 1
view_item_enabled = 2


class ListSelectorPresenter(object):
    def __init__(self, view, model):
        """
        Initialize with a view and model reference
        :param view: A reference to a view to control
        :param model: A reference to a dictionary to act as a model. The following structure is expected:
        {'name': [index_position, ticked, enabled]}
        """
        self.view = view
        self.model = model
        self.filter_string = ''
        self.filter_type = 'Include'
        self.filter_list = []
        self.show_only_selected = False

        self.view.set_filter_line_edit_changed_action(
            self.handle_filter_changed)
        self.view.set_item_selection_changed_action(
            self.handle_selection_changed)
        self.view.set_filter_type_combo_changed_action(self.set_filter_type)
        self.view.set_select_all_checkbox_action(
            self.handle_select_all_checkbox_changed)
        self.view.set_row_moved_checkbox_action(self.handle_row_moved)
        self.view.set_show_selected_checkbox_changed(
            self.handle_show_selected_checked)

    def update_view_from_model(self):
        filtered_list = self.get_filtered_list()

        self.view.clearItems()
        self.view.addItems(filtered_list)

        number_selected = len(
            [item for item in filtered_list if item[view_item_selected]])
        number_of_selected_displayed = len(
            [item for item in self.model.values() if item[model_selected]])
        self.view.update_number_of_selected_label(
            number_selected, number_of_selected_displayed)

    def handle_filter_changed(self, filter_string):
        self.filter_string = filter_string
        self.update_view_from_model()

    def handle_selection_changed(self, name, state):
        self.model[name][model_selected] = state
        self.update_view_from_model()

    def handle_select_all_checkbox_changed(self, state):
        for item in self.get_filtered_list():
            self.model[item[view_item_name]][model_selected] = state

        self.update_view_from_model()

    def get_selected_items(self):
        selected_items = [
            name for name in self.model if self.model[name][model_selected]
        ]
        selected_items.sort(key=lambda selected_item: self.model[selected_item]
                            [model_position])
        return selected_items

    def get_selected_items_and_positions(self):
        """
        :return: A list of 2-tuples containing the (name, list position)
        of each selected item. They are ordered by the position
        """
        selected_items = [(name, self.model[name][model_position])
                          for name in self.model
                          if self.model[name][model_selected]]
        selected_items.sort(key=lambda selected_item: self.model[selected_item[0]]
                            [model_position])
        return selected_items

    def set_filter_type(self):
        self.filter_type = self.view.filter_type_combo_box.currentText()
        self.update_view_from_model()

    def handle_row_moved(self, insertion_index, rows_moved):
        filtered_list = self.get_filtered_list()

        if insertion_index >= len(filtered_list):
            new_position = len(filtered_list)
        else:
            name_of_row_to_insert_before = self.get_filtered_list(
            )[insertion_index][view_item_name]
            new_position = self.model[name_of_row_to_insert_before][
                model_position]

        names_of_rows_moved = [
            self.get_filtered_list()[index][view_item_name]
            for index in rows_moved
        ]

        for index, row in enumerate(names_of_rows_moved):
            old_position = self.model[row][model_position] + index
            new_position_temp = new_position + index
            for name in self.model:
                if new_position_temp <= self.model[name][
                        model_position] < old_position:
                    self.model[name][model_position] += 1
                self.model[row][model_position] = new_position_temp

    def handle_show_selected_checked(self, check_state):
        if check_state:
            self.show_only_selected = True
            self.view.disable_filtering_options()
        else:
            self.show_only_selected = False
            self.view.enable_filtering_options()

        self.update_view_from_model()

    def get_filtered_list(self):
        if self.show_only_selected:
            filtered_list = [[name, vals[model_selected], vals[model_enabled]]
                             for (name, vals) in self.model.items()
                             if vals[model_selected]]
            filtered_list.sort(key=lambda item: self.model[item[view_item_name]
                                                           ][model_position])
            return filtered_list

        if self.filter_type == 'Include':
            filtered_list = [[name, vals[model_selected], vals[model_enabled]]
                             for (name, vals) in self.model.items()
                             if self.filter_string in name]
        else:
            filtered_list = [[name, vals[model_selected], vals[model_enabled]]
                             for (name, vals) in self.model.items()
                             if self.filter_string not in name]

        filtered_list.sort(
            key=lambda item: self.model[item[view_item_name]][model_position])
        filtered_list = [
            item for item in filtered_list
            if item[view_item_name] not in self.filter_list
        ]

        return filtered_list

    def update_filter_list(self, filter_list):
        self.filter_list = filter_list
        self.update_view_from_model()

    def update_model(self, new_model):
        self.model = new_model
        self.update_view_from_model()
