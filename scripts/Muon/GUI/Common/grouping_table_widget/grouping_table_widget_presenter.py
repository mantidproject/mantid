# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import re

from Muon.GUI.Common.utilities import run_string_utils as run_utils
from Muon.GUI.Common.muon_group import MuonGroup
from mantidqt.utils.observer_pattern import GenericObservable

maximum_number_of_groups = 20


class GroupingTablePresenter(object):

    def __init__(self, view, model):
        self._view = view
        self._model = model

        self._view.on_add_group_button_clicked(self.handle_add_group_button_clicked)
        self._view.on_remove_group_button_clicked(self.handle_remove_group_button_clicked)

        self._view.on_user_changes_group_name(self.validate_group_name)
        self._view.on_user_changes_detector_IDs(self.validate_detector_ids)

        self._view.on_table_data_changed(self.handle_data_change)

        self._view.on_user_changes_min_range_source(self.first_good_data_checkbox_changed)

        self._view.on_user_changes_max_range_source(self.from_file_checkbox_changed)

        self._view.on_user_changes_group_range_min_text_edit(self.handle_group_range_min_updated)

        self._view.on_user_changes_group_range_max_text_edit(self.handle_group_range_max_updated)

        self.selected_group_changed_notifier = GenericObservable()

        self._dataChangedNotifier = lambda: 0

    def show(self):
        self._view.show()

    def on_data_changed(self, notifier):
        self._dataChangedNotifier = notifier

    def notify_data_changed(self):
        self._dataChangedNotifier()

    def _is_edited_name_duplicated(self, new_name):
        is_name_column_being_edited = self._view.grouping_table.currentColumn() == 0
        is_name_unique = True
        if new_name in self._model.group_and_pair_names:
            is_name_unique = False
        return is_name_column_being_edited and not is_name_unique

    def validate_group_name(self, text):
        if self._is_edited_name_duplicated(text):
            self._view.warning_popup("Groups and pairs must have unique names")
            return False
        if not re.match(run_utils.valid_name_regex, text):
            self._view.warning_popup("Group names should only contain digits, characters and _")
            return False
        return True

    def validate_detector_ids(self, text):
        try:
            if re.match(run_utils.run_string_regex, text) and run_utils.run_string_to_list(text, False) and \
                    max(run_utils.run_string_to_list(text, False)) <= self._model._data.num_detectors \
                    and min(run_utils.run_string_to_list(text, False)) > 0:
                return True
        except OverflowError:
            pass

        self._view.warning_popup("Invalid detector list.")
        return False

    def disable_editing(self):
        self._view.disable_editing()

    def enable_editing(self):
        self._view.enable_editing()

    def add_group(self, group):
        """Adds a group to the model and view"""
        try:
            if self._view.num_rows() >= maximum_number_of_groups:
                self._view.warning_popup("Cannot add more than {} groups.".format(maximum_number_of_groups))
                return
            # self.add_group_to_view(group)
            self.add_group_to_model(group)
            self.update_view_from_model()
            self._view.notify_data_changed()
            self.notify_data_changed()
        except ValueError as error:
            self._view.warning_popup(error)

    def add_group_to_model(self, group):
        self._model.add_group(group)

    def add_group_to_view(self, group, state):
        self._view.disable_updates()
        assert isinstance(group, MuonGroup)
        entry = [str(group.name), state, run_utils.run_list_to_string(group.detectors, False), str(group.n_detectors)]
        self._view.add_entry_to_table(entry)
        self._view.enable_updates()

    def handle_add_group_button_clicked(self):
        new_group_name = self._view.enter_group_name()
        if new_group_name is None:
            return
        if new_group_name in self._model.group_and_pair_names:
            self._view.warning_popup("Groups and pairs must have unique names")
        elif self.validate_group_name(new_group_name):
            group = MuonGroup(group_name=str(new_group_name), detector_ids=[1])
            self.add_group(group)

    def handle_remove_group_button_clicked(self):
        group_names = self._view.get_selected_group_names()
        if not group_names:
            self.remove_last_row_in_view_and_model()
        else:
            self.remove_selected_rows_in_view_and_model(group_names)
        self._view.notify_data_changed()
        self.notify_data_changed()

    def remove_selected_rows_in_view_and_model(self, group_names):
        self._view.remove_selected_groups()
        for group_name in group_names:
            self._model.remove_group_from_analysis(group_name)
        self._model.remove_groups_by_name(group_names)

    def remove_last_row_in_view_and_model(self):
        if self._view.num_rows() > 0:
            name = self._view.get_table_contents()[-1][0]
            self._view.remove_last_row()
            self._model.remove_group_from_analysis(name)
            self._model.remove_groups_by_name([name])

    def handle_data_change(self, row, col):
        changed_item = self._view.get_table_item(row, col)
        group_name = self._view.get_table_item(row, 0).text()
        update_model = True
        if col == 0 and not self.validate_group_name(changed_item.text()):
            update_model = False
        if col == 2 and not self.validate_detector_ids(changed_item.text()):
            update_model = False
        if col == 1:
            update_model = False
            self.to_analyse_data_checkbox_changed(changed_item.checkState(), row, group_name)

        if update_model:
            try:
                self.update_model_from_view()
            except ValueError as error:
                self._view.warning_popup(error)

        # if the column containing the "to_analyse" flag is changed, then we don't need to update anything group related
        if col != 1:
            self.update_view_from_model()
            self._view.notify_data_changed()
            self.notify_data_changed()

    def update_model_from_view(self):
        table = self._view.get_table_contents()
        self._model.clear_groups()
        for entry in table:
            detector_list = run_utils.run_string_to_list(str(entry[2]), False)
            group = MuonGroup(group_name=str(entry[0]), detector_ids=detector_list)
            self._model.add_group(group)

    def update_view_from_model(self):
        self._view.disable_updates()
        self._view.clear()

        for group in self._model.groups:
            to_analyse = True if group.name in self._model.selected_groups else False
            self.add_group_to_view(group, to_analyse)

        if self._view.group_range_use_last_data.isChecked():
            self._view.group_range_max.setText(str(self._model.get_last_data_from_file()))

        if self._view.group_range_use_first_good_data.isChecked():
            self._view.group_range_min.setText(str(self._model.get_first_good_data_from_file()))

        self._view.enable_updates()

    def to_analyse_data_checkbox_changed(self, state, row, group_name):
        group_added = True if state == 2 else False
        if group_added:
            self._model.add_group_to_analysis(group_name)
        else:
            self._model.remove_group_from_analysis(group_name)

        self.selected_group_changed_notifier.notify_subscribers(group_added)

    def plot_default_case(self):
        for row in range(self._view.num_rows()):
            self._view.set_to_analyse_state_quietly(row, True)
            group_name = self._view.get_table_item(row, 0).text()
            self._model.add_group_to_analysis(group_name)

    def first_good_data_checkbox_changed(self):
        if self._view.group_range_use_first_good_data.isChecked():
            self._view.group_range_min.setText(str(self._model.get_first_good_data_from_file()))

            self._view.group_range_min.setEnabled(False)
            if 'GroupRangeMin' in self._model._context.gui_context:
                # Remove variable from model if value from file is to be used
                self._model._context.gui_context.pop('GroupRangeMin')
                self._model._context.gui_context.update_and_send_signal()
        else:
            self._view.group_range_min.setEnabled(True)
            self._model._context.gui_context.update_and_send_signal(
                GroupRangeMin=float(self._view.group_range_min.text()))

    def from_file_checkbox_changed(self):
        if self._view.group_range_use_last_data.isChecked():
            self._view.group_range_max.setText(str(self._model.get_last_data_from_file()))

            self._view.group_range_max.setEnabled(False)
            if 'GroupRangeMax' in self._model._context.gui_context:
                # Remove variable from model if value from file is to be used
                self._model._context.gui_context.pop('GroupRangeMax')
                self._model._context.gui_context.update_and_send_signal()

        else:
            self._view.group_range_max.setEnabled(True)
            self._model._context.gui_context.update_and_send_signal(
                GroupRangeMax=float(self._view.group_range_max.text()))

    def handle_group_range_min_updated(self):
        range_min_new = float(self._view.group_range_min.text())
        range_max_current = self._model._context.gui_context['GroupRangeMax'] if 'GroupRangeMax' in \
                                                                                 self._model._context.gui_context \
            else self._model.get_last_data_from_file()
        if range_min_new < range_max_current:
            self._model._context.gui_context.update_and_send_signal(GroupRangeMin=range_min_new)
        else:
            self._view.group_range_min.setText(str(self._model._context.gui_context['GroupRangeMin']))
            self._view.warning_popup('Minimum of group asymmetry range must be less than maximum')

    def handle_group_range_max_updated(self):
        range_max_new = float(self._view.group_range_max.text())
        range_min_current = self._model._context.gui_context['GroupRangeMin'] if 'GroupRangeMin' in \
                                                                                 self._model._context.gui_context \
            else self._model.get_first_good_data_from_file()
        if range_max_new > range_min_current:
            self._model._context.gui_context.update_and_send_signal(GroupRangeMax=range_max_new)
        else:
            self._view.group_range_max.setText(str(self._model._context.gui_context['GroupRangeMax']))
            self._view.warning_popup('Maximum of group asymmetry range must be greater than minimum')
