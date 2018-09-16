from __future__ import (absolute_import, division, print_function)

import re

from Muon.GUI.Common.muon_pair import MuonPair


class PairingTablePresenter(object):

    def __init__(self, view, model):
        self._view = view
        self._model = model

        self._view.on_add_pair_button_clicked(self.handle_add_pair_button_clicked)
        self._view.on_remove_pair_button_clicked(self.handle_remove_pair_button_clicked)

        self._view.on_user_changes_pair_name(self.validate_pair_name)
        self._view.on_user_changes_alpha(self.validate_alpha)

        self._view.on_table_data_changed(self.handle_data_change)

    def validate_pair_name(self, text):
        if sum(text == name for name in self._model.group_names) > 0 or sum(
                text == name for name in self._model.pair_names) > 1:
            self._view.warning_popup("Groups and pairs must have unique names")
            return False
        if not re.match("^\w+$", text):
            self._view.warning_popup("Pair names should only contain digits, characters and _")
            return False
        return True

    def validate_alpha(self, alpha_text):
        if not re.match("[+]?\d*\.\d+|\d+", alpha_text) or float(alpha_text < 0.0):
            self._view.warning_popup("Alpha must be > 0")
            return False
        return True

    def disable_editing(self):
        self._view.disable_editing()

    def enable_editing(self):
        self._view.enable_editing()

    def show(self):
        self._view.show()

    def add_pair(self, pair):
        print("Pairing : Add pair (no notify)")
        self.add_pair_to_view(pair)
        self._model.add_pair(pair)

    def add_pair_to_view(self, pair):
        self._view.disable_updates()
        self.update_group_selections()
        assert isinstance(pair, MuonPair)
        if self._view.num_rows() > 20:
            self._view.warning_popup("Cannot add more than 20 pairs.")
            self._view.enable_updates()
            return

        entry = [str(pair.name), str(pair.group1), str(pair.group2), str(pair.alpha)]
        self._view.add_entry_to_table(entry)
        self._view.enable_updates()

    def handle_add_pair_button_clicked(self):
        pair = self._model.construct_empty_pair(self._view.num_rows() + 1)
        self.add_pair(pair)

    def handle_remove_pair_button_clicked(self):
        print("Pairing : Remove pair (no notify)")
        pair_names = self._view.get_selected_pair_names()
        if not pair_names:
            self.remove_last_row_in_view_and_model()
        else:
            self._view.remove_selected_pairs()
            self._model.remove_pairs_by_name(pair_names)

    def remove_last_row_in_view_and_model(self):
        if self._view.num_rows() > 0:
            name = self._view.get_table_contents()[-1][0]
            self._view.remove_last_row()
            self._model.remove_pairs_by_name([name])

    def handle_data_change(self):
        print("Pairing : Handle data change (not notify)")
        self.update_model_from_view()
        self.update_view_from_model()
        # self._view.notify_data_changed()

    def update_model_from_view(self):
        table = self._view.get_table_contents()
        self._model.clear_pairs()
        for entry in table:
            pair = MuonPair(pair_name=str(entry[0]), group1_name=str(entry[1]), group2_name=str(entry[2]),
                            alpha=float(entry[3]))
            self._model.add_pair(pair)

    def update_view_from_model(self):
        self._view.disable_updates()

        self._view.clear()
        # self.update_group_selections()
        for pair in self._model.pairs:
            self.add_pair_to_view(pair)

        self._view.enable_updates()

    def update_group_selections(self):
        groups = self._model.group_names
        self._view.update_group_selections(groups)
