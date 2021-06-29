# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import re
from Muon.GUI.Common.utilities import run_string_utils as run_utils
from Muon.GUI.ElementalAnalysis2.ea_group import EAGroup
from mantidqt.utils.observer_pattern import GenericObservable
from Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_table_widget_view import INVERSE_GROUP_TABLE_COLUMNS

MAXIMUM_NUMBER_OF_GROUPS = 20
REBIN_NONE_OPTION = "0"
REBIN_FIXED_OPTION = "1"
REBIN_VARIABLE_OPTION = "2"


class EAGroupingTablePresenter(object):

    def __init__(self, view, model):
        self._view = view
        self._model = model

        self._view.on_remove_group_button_clicked(self.handle_remove_group_button_clicked)

        self._view.on_user_changes_group_name(self.validate_group_name)

        self._view.on_table_data_changed(self.handle_data_change)

        self.selected_group_changed_notifier = GenericObservable()

        self._dataChangedNotifier = lambda: 0

        self.rebin_notifier = GenericObservable()

        self.data_changed_notifier = GenericObservable()

    def notify_data_changed(self):
        self.data_changed_notifier.notify_subscribers()
        self._dataChangedNotifier()

    def _is_edited_name_duplicated(self, new_name):
        is_name_column_being_edited = self._view.grouping_table.currentColumn() == 0
        is_name_not_unique = new_name in self._model.group_names
        return is_name_column_being_edited and is_name_not_unique

    def validate_group_name(self, text):
        if self._is_edited_name_duplicated(text):
            self._view.warning_popup("Groups must have unique names")
            return False
        if not re.match(run_utils.valid_name_regex, text):
            self._view.warning_popup("Group names should only contain digits, characters and _")
            return False
        return True

    def disable_editing(self):
        self._view.disable_editing()

    def enable_editing(self):
        self._view.enable_editing()

    def add_group_to_view(self, group, state):
        self._view.disable_updates()
        assert isinstance(group, EAGroup)

        entry = [str(group._group_name), str(group.run_number), group.detector, state, str(group.rebin_index),
                 group.rebin_option]
        self._view.add_entry_to_table(entry)
        self._view.enable_updates()

    def handle_remove_group_button_clicked(self):
        group_names = self._view.get_selected_group_names()
        if not group_names:
            self.remove_last_row_in_view_and_model()
        else:
            self.remove_selected_rows_in_view_and_model(group_names)
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
        workspace_name = self._view.get_table_item(row, INVERSE_GROUP_TABLE_COLUMNS['workspace_name']).text()

        update_model = self.handle_to_analyse_column_changed(col, changed_item, workspace_name)
        self.handle_rebin_column_changed(col, row, changed_item)
        self.handle_rebin_option_column_changed(col, changed_item, workspace_name)

        self.handle_update(update_model)

    def handle_to_analyse_column_changed(self, col, changed_item, workspace_name):
        update_model = True
        if col == INVERSE_GROUP_TABLE_COLUMNS['to_analyse']:
            update_model = False
            self.to_analyse_data_checkbox_changed(changed_item.checkState(), workspace_name)
        return update_model

    def handle_rebin_column_changed(self, col, row, changed_item):
        if col == INVERSE_GROUP_TABLE_COLUMNS['rebin']:
            if changed_item.text() == REBIN_FIXED_OPTION:
                self._view.rebin_fixed_chosen(row)
            elif changed_item.text() == REBIN_VARIABLE_OPTION:
                self._view.rebin_variable_chosen(row)
            elif changed_item.text() == REBIN_NONE_OPTION:
                self._view.rebin_none_chosen(row)

    def handle_rebin_option_column_changed(self, col, changed_item, workspace_name):
        if col == INVERSE_GROUP_TABLE_COLUMNS['rebin_options']:
            params = changed_item.text().split(":")
            if len(params) == 2:
                if params[0] == "Steps":
                    self._model.handle_rebin(name=workspace_name, rebin_type="Fixed", rebin_param=float(params[1]))
                if params[0] == "Bin Boundaries":
                    if len(params[1]) >= 1:
                        self._model.handle_rebin(name=workspace_name, rebin_type="Variable", rebin_param=params[1])

    def handle_update(self, update_model):
        if not update_model:
            # Reset the view back to model values and exit early as the changes are invalid.
            self.update_view_from_model()
            self.notify_data_changed()
            return

    def update_model_from_view(self):
        table = self._view.get_table_contents()
        self._model.clear_groups()
        for entry in table:
            group = EAGroup(group_name=str(entry[0]), detector=str(entry[2]), run_number=str(entry[1]))
            if entry[4]:
                group.rebin_index = str(entry[4])
            else:
                group.rebin_index = 0
            group.rebin_option = str(entry[5])
            self._model.add_group_from_table(group)

    def update_view_from_model(self):
        self._view.disable_updates()
        self._view.clear()

        for group in self._model.groups:
            if self._view.num_rows() >= MAXIMUM_NUMBER_OF_GROUPS:
                self._view.warning_popup("Cannot add more than {} groups.".format(MAXIMUM_NUMBER_OF_GROUPS))
                break
            else:
                to_analyse = True if group.name in self._model.selected_groups else False
                self.add_group_to_view(group, to_analyse)

        self._view.enable_updates()

    def to_analyse_data_checkbox_changed(self, state, group_name):
        group_added = True if state == 2 else False

        if group_added:
            self._model.add_group_to_analysis(group_name)
        else:
            self._model.remove_group_from_analysis(group_name)

        group_info = {'is_added': group_added, 'name': group_name}
        self.selected_group_changed_notifier.notify_subscribers(group_info)

    def plot_default_case(self):
        for row in range(self._view.num_rows()):
            self._view.set_to_analyse_state_quietly(row, True)
            group_name = self._view.get_table_item(row, 0).text()
            self._model.add_group_to_analysis(group_name)
