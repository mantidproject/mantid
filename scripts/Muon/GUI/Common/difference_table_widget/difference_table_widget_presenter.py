# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import re

from Muon.GUI.Common.muon_diff import MuonDiff
from Muon.GUI.Common.utilities.run_string_utils import valid_name_regex
from mantidqt.utils.observer_pattern import GenericObservable
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import RowValid
from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_presenter import row_colors, row_tooltips

diff_columns = ['diff_name', 'to_analyse', 'group_1', 'group_2']


class DifferenceTablePresenter(object):
    def __init__(self, view, model, group_or_pair):
        self._view = view
        self._model = model
        self._group_or_pair = group_or_pair
        self._view.on_add_diff_button_clicked(self.handle_add_diff_button_checked_state)
        self._view.on_remove_diff_button_clicked(self.handle_remove_diff_button_clicked)

        self._view.on_user_changes_diff_name(self.validate_diff_name)
        self._view.on_table_data_changed(self.handle_data_change)

        self.selected_diff_changed_notifier = GenericObservable()

        self._dataChangedNotifier = lambda: 0

        if group_or_pair == 'pair':
            self._view.set_table_headers_pairs()
        else:
            self._view.set_table_headers_groups()

    def show(self):
        self._view.show()

    def on_data_changed(self, notifier):
        self._dataChangedNotifier = notifier

    def notify_data_changed(self):
        self._dataChangedNotifier()

    def disable_editing(self):
        self._view.disable_editing()

    def enable_editing(self):
        self._view.enable_editing()

    def handle_data_change(self, row, col):
        table = self._view.get_table_contents()
        changed_item = self._view.get_table_item(row, col)
        changed_item_text = self._view.get_table_item_text(row, col)
        diff_name = self._view.get_table_item_text(row, 0)
        update_model = True
        if diff_columns[col] == 'diff_name' and not self.validate_diff_name(changed_item_text):
            update_model = False
        if diff_columns[col] == 'group_1':
            if changed_item_text == self._view.get_table_item_text(row, diff_columns.index('group_2')):
                table[row][diff_columns.index('group_2')] = self._model.get_diffs(self._group_or_pair)[row].positive
        if diff_columns[col] == 'group_2':
            if changed_item_text == self._view.get_table_item_text(row, diff_columns.index('group_1')):
                table[row][diff_columns.index('group_1')] = self._model.get_diffs(self._group_or_pair)[row].negative
        if diff_columns[col] == 'to_analyse':
            update_model = False
            self.to_analyse_data_checkbox_changed(changed_item.checkState(), row, diff_name)

        if update_model:
            self.update_model_from_view(table)

        if col != 1:
            self.update_view_from_model()
            self.notify_data_changed()

    def update_model_from_view(self, table=None):
        if not table:
            table = self._view.get_table_contents()
        self._model.clear_diffs(self._group_or_pair)
        for entry in table:
            periods = self._model._context.group_pair_context[
                entry[2]].periods + self._model._context.group_pair_context[entry[3]].periods
            diff = MuonDiff(diff_name=str(entry[0]),
                            positive=str(entry[2]),
                            negative=str(entry[3]),
                            group_or_pair=self._group_or_pair,
                            periods=periods)
            self._model.add_diff(diff)

    def update_view_from_model(self):
        self._view.disable_updates()

        self._view.clear()
        for diff in self._model.diffs:
            if isinstance(diff, MuonDiff) and diff.group_or_pair == self._group_or_pair:
                to_analyse = True if diff.name in self._model.selected_diffs else False
                positive_periods = self._model._context.group_pair_context[diff.positive].periods
                negative_periods = self._model._context.group_pair_context[diff.negative].periods
                forward_period_warning = self._model.validate_periods_list(positive_periods)
                backward_period_warning = self._model.validate_periods_list(negative_periods)
                if forward_period_warning == RowValid.invalid_for_all_runs or\
                        backward_period_warning == RowValid.invalid_for_all_runs:
                    display_period_warning = RowValid.invalid_for_all_runs
                elif forward_period_warning == RowValid.valid_for_some_runs or\
                        backward_period_warning == RowValid.valid_for_some_runs:
                    display_period_warning = RowValid.valid_for_some_runs
                else:
                    display_period_warning = RowValid.valid_for_all_runs
                color = row_colors[display_period_warning]
                tool_tip = row_tooltips[display_period_warning]
                self.add_diff_to_view(diff, to_analyse, color, tool_tip)

        self._view.enable_updates()

    def update_group_selections(self):
        groups = self._model.get_names(self._group_or_pair)
        self._view.update_group_selections(groups)

    def to_analyse_data_checkbox_changed(self, state, row, diff_name):
        diff_added = True if state == 2 else False
        if diff_added:
            self._model.add_diff_to_analysis(diff_name)
        else:
            self._model.remove_diff_from_analysis(diff_name)
        diff_info = {'is_added': diff_added, 'name': diff_name}
        self.selected_diff_changed_notifier.notify_subscribers(diff_info)

    def plot_default_case(self):
        for row in range(self._view.num_rows()):
            self._view.set_to_analyse_state_quietly(row, True)
            diff_name = self._view.get_table_item(row, 0).text()
            self._model.add_diff_to_analysis(diff_name)

    # ------------------------------------------------------------------------------------------------------------------
    # Add / Remove diffs
    # ------------------------------------------------------------------------------------------------------------------

    def add_diff(self, diff):
        """Add a diff to the model and view"""
        if self._view.num_rows() > 19:
            self._view.warning_popup("Cannot add more than 20 diffs.")
            return
        self.add_diff_to_model(diff)
        self.update_view_from_model()

    def add_diff_to_model(self, diff):
        self._model.add_diff(diff)

    def add_diff_to_view(self,
                         diff,
                         to_analyse=False,
                         color=row_colors[RowValid.valid_for_all_runs],
                         tool_tip=row_tooltips[RowValid.valid_for_all_runs]):
        self._view.disable_updates()
        self.update_group_selections()
        assert isinstance(diff, MuonDiff)
        entry = [str(diff.name), to_analyse, str(diff.positive), str(diff.negative)]
        self._view.add_entry_to_table(entry, color, tool_tip)
        self._view.enable_updates()

    """
    This is required to strip out the boolean value the clicked method
    of QButton emits by default.
    """

    def handle_add_diff_button_checked_state(self):
        self.handle_add_diff_button_clicked()

    def handle_add_diff_button_clicked(self, group_1='', group_2=''):
        if len(self._model.get_names(self._group_or_pair)) == 0 or len(self._model.get_names(self._group_or_pair)) == 1:
            self._view.warning_popup("At least two groups/pairs are required to create a diff")
        else:
            new_diff_name = self._view.enter_diff_name()
            if new_diff_name is None:
                return
            elif new_diff_name in self._model.group_and_pair_names:
                self._view.warning_popup("Groups and diffs must have unique names")
            elif self.validate_diff_name(new_diff_name):
                group1 = self._model.get_names(self._group_or_pair)[0] if not group_1 else group_1
                group2 = self._model.get_names(self._group_or_pair)[1] if not group_2 else group_2
                periods = self._model._context.group_pair_context[
                    group1].periods + self._model._context.group_pair_context[group2].periods
                diff = MuonDiff(diff_name=str(new_diff_name),
                                positive=group1,
                                negative=group2,
                                group_or_pair=self._group_or_pair,
                                periods=periods)
                self.add_diff(diff)
                self.notify_data_changed()

    def handle_remove_diff_button_clicked(self):
        diff_names = self._view.get_selected_diff_names()
        if not diff_names:
            self.remove_last_row_in_view_and_model()
        else:
            self._view.remove_selected_diffs()
            for diff_name in diff_names:
                self._model.remove_diff_from_analysis(diff_name)
            self._model.remove_diffs_by_name(diff_names)
        self.notify_data_changed()

    def remove_last_row_in_view_and_model(self):
        if self._view.num_rows() > 0:
            name = self._view.get_table_contents()[-1][0]
            self._view.remove_last_row()
            self._model.remove_diff_from_analysis(name)
            self._model.remove_diffs_by_name([name])

    # ------------------------------------------------------------------------------------------------------------------
    # Table entry validation
    # ------------------------------------------------------------------------------------------------------------------

    def _is_edited_name_duplicated(self, new_name):
        is_name_column_being_edited = self._view.diff_table.currentColumn() == 0
        is_name_unique = (sum([new_name == name for name in self._model.group_and_pair_names]) == 0)
        return is_name_column_being_edited and not is_name_unique

    def validate_diff_name(self, text):
        if self._is_edited_name_duplicated(text):
            self._view.warning_popup("Groups and diffs must have unique names")
            return False
        if not re.match(valid_name_regex, text):
            self._view.warning_popup("diff names should only contain digits, characters and _")
            return False
        return True
