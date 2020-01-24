# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import re

from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.utilities.run_string_utils import valid_name_regex, valid_alpha_regex
from mantidqt.utils.observer_pattern import Observable, GenericObservable

pair_columns = ['pair_name', 'to_analyse', 'group_1', 'group_2', 'alpha']


class PairingTablePresenter(object):

    def __init__(self, view, model):
        self._view = view
        self._model = model

        self._view.on_add_pair_button_clicked(self.handle_add_pair_button_clicked)
        self._view.on_remove_pair_button_clicked(self.handle_remove_pair_button_clicked)

        self._view.on_user_changes_pair_name(self.validate_pair_name)
        self._view.on_user_changes_alpha(self.validate_alpha)
        self._view.on_guess_alpha_clicked(self.handle_guess_alpha_clicked)
        self._view.on_table_data_changed(self.handle_data_change)

        self.selected_pair_changed_notifier = GenericObservable()

        self._dataChangedNotifier = lambda: 0
        self._on_alpha_clicked = lambda: 0
        self._on_guess_alpha_requested = lambda pair_name, group1, group2: 0

        # notify if Guess Alpha clicked for any table entries
        self.guessAlphaNotifier = PairingTablePresenter.GuessAlphaNotifier(self)

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

    def handle_guess_alpha_clicked(self, row):
        table_row = self._view.get_table_contents()[row]
        pair_name = table_row[0]
        group1 = table_row[2]
        group2 = table_row[3]
        self.guessAlphaNotifier.notify_subscribers([pair_name, group1, group2])

    def handle_data_change(self, row, col):
        table = self._view.get_table_contents()
        changed_item = self._view.get_table_item(row, col)
        changed_item_text = self._view.get_table_item_text(row, col)
        pair_name = self._view.get_table_item_text(row, 0)
        update_model = True
        if pair_columns[col] == 'pair_name' and not self.validate_pair_name(changed_item_text):
            update_model = False
        if pair_columns[col] == 'group_1':
            if changed_item_text == self._view.get_table_item_text(row, pair_columns.index('group_2')):
                table[row][pair_columns.index('group_2')] = self._model.pairs[row].forward_group
        if pair_columns[col] == 'group_2':
            if changed_item_text == self._view.get_table_item_text(row, pair_columns.index('group_1')):
                table[row][pair_columns.index('group_1')] = self._model.pairs[row].backward_group
        if pair_columns[col] == 'alpha':
            if not self.validate_alpha(changed_item_text):
                update_model = False
            else:
                rounded_item = '{:.3f}'.format(float(changed_item_text)) if '{:.3f}'.format(
                    float(changed_item_text)) != '0.000' \
                    else '{:.3g}'.format(float(changed_item_text))
                table[row][col] = rounded_item
        if pair_columns[col] == 'to_analyse':
            update_model = False
            self.to_analyse_data_checkbox_changed(changed_item.checkState(), row, pair_name)

        if update_model:
            self.update_model_from_view(table)

        if col != 1:
            self.update_view_from_model()
            self.notify_data_changed()

    def update_model_from_view(self, table=None):
        if not table:
            table = self._view.get_table_contents()
        self._model.clear_pairs()
        for entry in table:
            pair = MuonPair(pair_name=str(entry[0]),
                            backward_group_name=str(entry[3]),
                            forward_group_name=str(entry[2]),
                            alpha=float(entry[4]))
            self._model.add_pair(pair)

    def update_view_from_model(self):
        self._view.disable_updates()

        self._view.clear()
        for pair in self._model.pairs:
            to_analyse = True if pair.name in self._model.selected_pairs else False
            self.add_pair_to_view(pair, to_analyse)

        self._view.enable_updates()

    def update_group_selections(self):
        groups = self._model.group_names
        self._view.update_group_selections(groups)

    def to_analyse_data_checkbox_changed(self, state, row, pair_name):
        pair_added = True if state == 2 else False
        if pair_added:
            self._model.add_pair_to_analysis(pair_name)
        else:
            self._model.remove_pair_from_analysis(pair_name)
        self.selected_pair_changed_notifier.notify_subscribers(state)

    def plot_default_case(self):
        for row in range(self._view.num_rows()):
            self._view.set_to_analyse_state(row, True)

    # ------------------------------------------------------------------------------------------------------------------
    # Add / Remove pairs
    # ------------------------------------------------------------------------------------------------------------------

    def add_pair(self, pair):
        """Add a pair to the model and view"""
        if self._view.num_rows() > 19:
            self._view.warning_popup("Cannot add more than 20 pairs.")
            return
        self.add_pair_to_view(pair)
        self.add_pair_to_model(pair)

    def add_pair_to_model(self, pair):
        self._model.add_pair(pair)

    def add_pair_to_view(self, pair, to_analyse=False):
        self._view.disable_updates()
        self.update_group_selections()
        assert isinstance(pair, MuonPair)
        entry = [str(pair.name), to_analyse, str(pair.forward_group), str(pair.backward_group), str(pair.alpha)]
        self._view.add_entry_to_table(entry)
        self._view.enable_updates()

    def handle_add_pair_button_clicked(self, group_1='', group_2=''):
        if len(self._model.group_names) == 0 or len(self._model.group_names) == 1:
            self._view.warning_popup("At least two groups are required to create a pair")
        else:
            new_pair_name = self._view.enter_pair_name()
            if new_pair_name is None:
                return
            elif new_pair_name in self._model.group_and_pair_names:
                self._view.warning_popup("Groups and pairs must have unique names")
            elif self.validate_pair_name(new_pair_name):
                group1 = self._model.group_names[0] if not group_1 else group_1
                group2 = self._model.group_names[1] if not group_2 else group_2
                pair = MuonPair(pair_name=str(new_pair_name),
                                forward_group_name=group1, backward_group_name=group2, alpha=1.0)
                self.add_pair(pair)
                self.notify_data_changed()

    def handle_remove_pair_button_clicked(self):
        pair_names = self._view.get_selected_pair_names()
        if not pair_names:
            self.remove_last_row_in_view_and_model()
        else:
            self._view.remove_selected_pairs()
            for pair_name in pair_names:
                self._model.remove_pair_from_analysis(pair_name)
            self._model.remove_pairs_by_name(pair_names)
        self.notify_data_changed()

    def remove_last_row_in_view_and_model(self):
        if self._view.num_rows() > 0:
            name = self._view.get_table_contents()[-1][0]
            self._view.remove_last_row()
            self._model.remove_pair_from_analysis(name)
            self._model.remove_pairs_by_name([name])

    # ------------------------------------------------------------------------------------------------------------------
    # Table entry validation
    # ------------------------------------------------------------------------------------------------------------------

    def _is_edited_name_duplicated(self, new_name):
        is_name_column_being_edited = self._view.pairing_table.currentColumn() == 0
        is_name_unique = (sum(
            [new_name == name for name in self._model.group_and_pair_names]) == 0)
        return is_name_column_being_edited and not is_name_unique

    def validate_pair_name(self, text):
        if self._is_edited_name_duplicated(text):
            self._view.warning_popup("Groups and pairs must have unique names")
            return False
        if not re.match(valid_name_regex, text):
            self._view.warning_popup("Pair names should only contain digits, characters and _")
            return False
        return True

    def validate_alpha(self, alpha_text):
        if not re.match(valid_alpha_regex, alpha_text) or float(alpha_text) <= 0.0:
            self._view.warning_popup("Alpha must be > 0")
            return False
        return True

    # ------------------------------------------------------------------------------------------------------------------
    # Observer / Observable
    # ------------------------------------------------------------------------------------------------------------------

    class GuessAlphaNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer  # handle to containing class

        def notify_subscribers(self, arg=["", "", ""]):
            Observable.notify_subscribers(self, arg)
