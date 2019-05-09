# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
class ListSelectorPresenter(object):
    def __init__(self, view, model):
        self.view = view
        self.model = model
        self.filter_string = ''
        self.filter_type = 'Include'

        self.view.set_filter_line_edit_changed_action(self.handle_filter_changed)
        self.view.set_item_selection_changed_action(self.handle_selection_changed)
        self.view.set_filter_type_combo_changed_action(self.set_filter_type)
        self.view.set_select_all_checkbox_action(self.handle_select_all_checkbox_changed)
        self.view.set_row_moved_checkbox_action(self.handle_row_moved)

    def update_view_from_model(self):
        filtered_list = self.get_filtered_list()

        self.view.clearItems()
        self.view.addItems(filtered_list)

    def handle_filter_changed(self, filter_string):
        self.filter_string = filter_string
        self.update_view_from_model()

    def handle_selection_changed(self, name, state):
        self.model[name][1] = state

    def handle_select_all_checkbox_changed(self, state):
        for item in self.get_filtered_list():
            item[1] = state

        self.update_view_from_model()

    def get_selected_items(self):
        selected_items = [name for name in self.model if self.model[name][1]]
        selected_items.sort(key=lambda val: self.model[val][0])
        return selected_items

    def set_filter_type(self):
        self.filter_type = self.view.filter_type_combo_box.currentText()
        self.update_view_from_model()

    def handle_row_moved(self, insertion_index, rows_moved):
        filtered_list = self.get_filtered_list()

        if insertion_index >= len(filtered_list):
            new_position = len(filtered_list)
        else:
            name_of_row_to_insert_before = self.get_filtered_list()[insertion_index][0]
            new_position = self.model[name_of_row_to_insert_before][0]

        names_of_rows_moved = [self.get_filtered_list()[index][0] for index in rows_moved]

        for index, row in enumerate(names_of_rows_moved):
            old_position = self.model[row][0] + index
            new_position_temp = new_position + index
            for name in self.model:
                if new_position_temp <= self.model[name][0] < old_position:
                    self.model[name][0] += 1
                self.model[row][0] = new_position_temp

    def get_filtered_list(self):
        if self.filter_type == 'Include':
            filtered_list = [[item, vals[1], vals[2]] for (item, vals) in self.model.items() if
                             self.filter_string in item]
            filtered_list.sort(key=lambda val: self.model[val[0]][0])
        else:
            filtered_list = [[item, vals[1], vals[2]] for (item, vals) in self.model.items() if
                             self.filter_string not in item]
            filtered_list.sort(key=lambda val: self.model[val[0]][0])

        return filtered_list
