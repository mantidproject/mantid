from __future__ import (absolute_import, division, print_function)

import re

from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.run_string_utils import valid_name_regex, valid_alpha_regex
from Muon.GUI.Common.observer_pattern import Observer, Observable


class PairingTablePresenter(object):

    def __init__(self, view, model):
        self._view = view
        self._model = model

        self._view.on_add_pair_button_clicked(self.handle_add_pair_button_clicked)
        self._view.on_remove_pair_button_clicked(self.handle_remove_pair_button_clicked)

        self._view.on_user_changes_pair_name(self.validate_pair_name)
        self._view.on_user_changes_alpha(self.validate_alpha)

        self._view.on_table_data_changed(self.handle_data_change)

        self._dataChangedNotifier = lambda: 0

        self._on_alpha_clicked = lambda: 0
        self._on_guess_alpha_requested = lambda pair_name, group1, group2: 0

        self.guessAlphaNotifier = PairingTablePresenter.GuessAlphaNotifier(self)

        self._view.on_guess_alpha_clicked(self.handle_guess_alpha_clicked)

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

    def add_pair(self, pair):
        """Add a pair to the model and view"""
        if self._view.num_rows() > 19:
            self._view.warning_popup("Cannot add more than 20 pairs.")
            return
        self.add_pair_to_view(pair)
        self.add_pair_to_model(pair)

    def add_pair_to_model(self, pair):
        self._model.add_pair(pair)

    def add_pair_to_view(self, pair):
        self._view.disable_updates()
        self.update_group_selections()
        assert isinstance(pair, MuonPair)
        entry = [str(pair.name), str(pair.group1), str(pair.group2), str(pair.alpha)]
        self._view.add_entry_to_table(entry)
        self._view.enable_updates()

    def handle_add_pair_button_clicked(self):
        pair = self._model.construct_empty_pair(self._view.num_rows() + 1)
        self.add_pair(pair)
        self.notify_data_changed()

    def handle_remove_pair_button_clicked(self):
        pair_names = self._view.get_selected_pair_names()
        if not pair_names:
            self.remove_last_row_in_view_and_model()
        else:
            self._view.remove_selected_pairs()
            self._model.remove_pairs_by_name(pair_names)
        self.notify_data_changed()

    def remove_last_row_in_view_and_model(self):
        if self._view.num_rows() > 0:
            name = self._view.get_table_contents()[-1][0]
            self._view.remove_last_row()
            self._model.remove_pairs_by_name([name])

    def handle_guess_alpha_clicked(self, row):
        table_row = self._view.get_table_contents()[row]
        pair_name = table_row[0]
        group1 = table_row[1]
        group2 = table_row[2]
        self.guessAlphaNotifier.notify_subscribers([pair_name, group1, group2])

    def handle_data_change(self):
        self.update_model_from_view()
        self.update_view_from_model()
        self.notify_data_changed()

    def update_model_from_view(self):
        table = self._view.get_table_contents()
        self._model.clear_pairs()
        for entry in table:
            print("ENTRY : ", entry)
            pair = MuonPair(pair_name=str(entry[0]),
                            group1_name=str(entry[1]),
                            group2_name=str(entry[2]),
                            alpha=float(entry[3]))
            self._model.add_pair(pair)

    def update_view_from_model(self):
        self._view.disable_updates()

        self._view.clear()
        for pair in self._model.pairs:
            self.add_pair_to_view(pair)

        self._view.enable_updates()

    def update_group_selections(self):
        groups = self._model.group_names
        self._view.update_group_selections(groups)

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
        if not re.match(valid_alpha_regex, alpha_text) or float(alpha_text) < 0.0:
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
