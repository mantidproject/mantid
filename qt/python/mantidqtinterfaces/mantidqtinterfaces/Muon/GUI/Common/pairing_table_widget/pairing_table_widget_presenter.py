# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import re

from mantidqtinterfaces.Muon.GUI.Common.muon_pair import MuonPair
from mantidqtinterfaces.Muon.GUI.Common.utilities.run_string_utils import valid_name_regex, valid_alpha_regex
from mantidqtinterfaces.Muon.GUI.Common.utilities.general_utils import round_value
from mantidqt.utils.observer_pattern import GenericObservable
from mantidqtinterfaces.Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import RowValid
from mantidqtinterfaces.Muon.GUI.Common.grouping_table_widget.grouping_table_widget_presenter import row_colors, row_tooltips

from mantidqtinterfaces.Muon.GUI.Common.pairing_table_widget.pairing_table_constants import get_pair_columns, pair_columns
from mantidqtinterfaces.Muon.GUI.Common.utilities import table_utils


class PairingTablePresenter(object):
    def __init__(self, view, model):
        self._view = view
        self._model = model

        self._view.on_add_pair_button_clicked(self.handle_add_pair_button_checked_state)
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
        self.guessAlphaNotifier = GenericObservable()

    def show(self):
        self._view.show()

    def on_data_changed(self, notifier):
        self._dataChangedNotifier = notifier

    def notify_data_changed(self):
        self._dataChangedNotifier()

    def disable_editing(self):
        self.disable_updates()
        self._view.is_disabled = True
        self._view.disable_all_buttons()
        self._disable_all_table_items()
        self.enable_updates()

    def enable_editing(self):
        self.disable_updates()
        self._view.is_disabled = False
        self._view.enable_all_buttons()
        self._enable_all_table_items()
        self.enable_updates()

    def enable_updates(self):
        """Allow update signals to be sent."""
        self._view._updating = False

    def disable_updates(self):
        """Prevent update signals being sent."""
        self._view._updating = True

    def num_rows(self):
        return self._view.get_pairing_table.rowCount()

    def num_cols(self):
        return self._view.get_pairing_table.columnCount()

    def remove_pair_by_index(self, index):
        self._view.get_pairing_table.removeRow(index)

    def remove_last_row(self):
        last_row = self._view.get_pairing_table.rowCount() - 1
        if last_row >= 0:
            self._view.get_pairing_table.removeRow(last_row)

    def clear(self):
        # Go backwards to preserve indices
        for row in reversed(range(self.num_rows())):
            self._view.get_pairing_table.removeRow(row)

    def get_selected_row_indices(self):
        return list(set(index.row() for index in self._view.get_pairing_table.selectedIndexes()))

    def get_selected_pair_names_and_indexes(self):
        indexes = self.get_selected_row_indices()
        return [[str(self._view.get_pairing_table.item(i, 0).text()), i] for i in indexes]

    def handle_guess_alpha_clicked(self, row):
        table_row = self.get_table_contents()[row]
        pair_name = table_row[0]
        group1 = table_row[2]
        group2 = table_row[3]
        self.guessAlphaNotifier.notify_subscribers([pair_name, group1, group2])

    def handle_data_change(self, row, col):
        table = self.get_table_contents()
        changed_item = self._view.get_table_item(row, col)
        changed_item_text = self.get_table_item_text(row, col)
        pair_name = self.get_table_item_text(row, 0)
        update_model = True
        if get_pair_columns()[col] == "pair_name" and not self.validate_pair_name(changed_item_text):
            update_model = False
        if get_pair_columns()[col] == "group_1":
            if changed_item_text == self.get_table_item_text(row, get_pair_columns().index("group_2")):
                table[row][get_pair_columns().index("group_2")] = self._model.pairs[row].forward_group
        if get_pair_columns()[col] == "group_2":
            if changed_item_text == self.get_table_item_text(row, get_pair_columns().index("group_1")):
                table[row][get_pair_columns().index("group_1")] = self._model.pairs[row].backward_group
        if get_pair_columns()[col] == "alpha":
            if not self.validate_alpha(changed_item_text):
                update_model = False
            else:
                rounded_item = round_value(changed_item_text, self._model._context.group_pair_context.alpha_precision)
                table[row][col] = rounded_item
        if get_pair_columns()[col] == "to_analyse":
            update_model = False
            self.to_analyse_data_checkbox_changed(changed_item.checkState(), row, pair_name)

        if update_model:
            self.update_model_from_view(table)

        if col != 1:
            self.update_view_from_model()
            self.notify_data_changed()

    def update_model_from_view(self, table=None):
        if not table:
            table = self.get_table_contents()
        self._model.clear_pairs()
        for entry in table:
            periods = self._model.get_periods(str(entry[2])) + self._model.get_periods(str(entry[3]))
            pair = MuonPair(
                pair_name=str(entry[0]),
                backward_group_name=str(entry[3]),
                forward_group_name=str(entry[2]),
                alpha=float(entry[4]),
                periods=periods,
            )
            self._model.add_pair(pair)

    def update_view_from_model(self):
        self.disable_updates()

        self.clear()
        for pair in self._model.pairs:
            if isinstance(pair, MuonPair):
                to_analyse = True if pair.name in self._model.selected_pairs else False
                forward_group_periods = self._model._context.group_pair_context[pair.forward_group].periods
                backward_group_periods = self._model._context.group_pair_context[pair.backward_group].periods
                forward_period_warning = self._model.validate_periods_list(forward_group_periods)
                backward_period_warning = self._model.validate_periods_list(backward_group_periods)
                if forward_period_warning == RowValid.invalid_for_all_runs or backward_period_warning == RowValid.invalid_for_all_runs:
                    display_period_warning = RowValid.invalid_for_all_runs
                elif forward_period_warning == RowValid.valid_for_some_runs or backward_period_warning == RowValid.valid_for_some_runs:
                    display_period_warning = RowValid.valid_for_some_runs
                else:
                    display_period_warning = RowValid.valid_for_all_runs
                color = row_colors[display_period_warning]
                tool_tip = row_tooltips[display_period_warning]
                self.add_pair_to_view(pair, to_analyse, color, tool_tip)

        self.enable_updates()

    def update_group_selections(self):
        groups = self._model.group_names + [diff.name for diff in self._model.get_diffs("group")]
        self._view.update_group_selections(groups)

    def to_analyse_data_checkbox_changed(self, state, row, pair_name):
        pair_added = True if state == 2 else False
        if pair_added:
            self._model.add_pair_to_analysis(pair_name)
        else:
            self._model.remove_pair_from_analysis(pair_name)
        pair_info = {"is_added": pair_added, "name": pair_name}
        self.selected_pair_changed_notifier.notify_subscribers(pair_info)

    def plot_default_case(self):
        for row in range(self.num_rows()):
            self._view.set_to_analyse_state_quietly(row, True)
            pair_name = self._view.get_table_item(row, 0).text()
            self._model.add_pair_to_analysis(pair_name)

    # ------------------------------------------------------------------------------------------------------------------
    # Add / Remove pairs
    # ------------------------------------------------------------------------------------------------------------------

    def add_pair(self, pair):
        """Add a pair to the model and view"""
        if self.num_rows() > 19:
            self._view.warning_popup("Cannot add more than 20 pairs.")
            return
        self.add_pair_to_model(pair)
        self.update_view_from_model()

    def add_pair_to_model(self, pair):
        self._model.add_pair(pair)

    def add_pair_to_view(
        self, pair, to_analyse=False, color=row_colors[RowValid.valid_for_all_runs], tool_tip=row_tooltips[RowValid.valid_for_all_runs]
    ):
        self.disable_updates()
        self.update_group_selections()
        assert isinstance(pair, MuonPair)
        entry = [str(pair.name), to_analyse, str(pair.forward_group), str(pair.backward_group), str(pair.alpha)]
        self.add_entry_to_table(entry, color, tool_tip)
        self.enable_updates()

    """
    This is required to strip out the boolean value the clicked method
    of QButton emits by default.
    """

    def handle_add_pair_button_checked_state(self):
        self.handle_add_pair_button_clicked()

    def handle_add_pair_button_clicked(self, group_1="", group_2=""):
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
                periods = self._model.get_periods(group1) + self._model.get_periods(group2)
                pair = MuonPair(
                    pair_name=str(new_pair_name), forward_group_name=group1, backward_group_name=group2, alpha=1.0, periods=periods
                )
                self.add_pair(pair)
                self.notify_data_changed()

    def handle_remove_pair_button_clicked(self):
        pair_names = self.get_selected_pair_names_and_indexes()
        if not pair_names:
            self.remove_last_row_in_view_and_model()
        else:
            self.remove_selected_rows_in_view_and_model(pair_names)
        self.notify_data_changed()

    def remove_selected_rows_in_view_and_model(self, pair_names):
        safe_to_rm = []
        warnings = ""
        for name, index in pair_names:
            used_by = self._model.check_if_used_by_diff(name)
            if used_by:
                warnings += used_by + "\n"
            else:
                safe_to_rm.append([index, name])
        for index, name in reversed(safe_to_rm):
            self._model.remove_pair_from_analysis(name)
            self.remove_pair_by_index(index)
        self._model.remove_pairs_by_name([name for index, name in safe_to_rm])
        if warnings:
            self._view.warning_popup(warnings)

    def remove_last_row_in_view_and_model(self):
        if self.num_rows() > 0:
            name = self.get_table_contents()[-1][0]
            warning = self._model.check_if_used_by_diff(name)
            if warning:
                self._view.warning_popup(warning)
            else:
                self.remove_last_row()
                self._model.remove_pair_from_analysis(name)
                self._model.remove_pairs_by_name([name])

    def get_table_item_text(self, row, col):
        column_name = pair_columns[col]
        if column_name == "group_1" or column_name == "group_2":
            return self._view.get_widget_current_text(row, col)
        elif column_name == "guess_alpha":
            return "Guess"
        else:
            return self._view.get_item_text(row, col)

    def get_table_contents(self):
        if self._view.is_updating:
            return []

        ret = [[None for _ in range(self.num_cols())] for _ in range(self.num_rows())]
        for row in range(self.num_rows()):
            for col in range(self.num_cols()):
                column_name = pair_columns[col]
                if column_name == "group_1" or column_name == "group_2":
                    ret[row][col] = self._view.get_widget_current_text(row, col)
                elif column_name == "guess_alpha":
                    ret[row][col] = "Guess"
                else:
                    ret[row][col] = self._view.get_item_text(row, col)
        return ret

    # ------------------------------------------------------------------------------------------------------------------
    # Enabling / Disabling the table
    # ------------------------------------------------------------------------------------------------------------------
    def _disable_all_table_items(self):
        for row in range(self.num_rows()):
            for col in range(self.num_cols()):
                column_name = pair_columns[col]
                if column_name in ["group_1", "group_2", "guess_alpha"]:
                    self._view.set_widget_enabled(row, col, False)
                else:
                    self._view.set_item_selectable(row, col)

    def _enable_all_table_items(self):
        for row in range(self.num_rows()):
            for col in range(self.num_cols()):
                column_name = pair_columns[col]
                if column_name in ["group_1", "group_2", "guess_alpha"]:
                    self._view.set_widget_enabled(row, col, True)
                elif column_name == "pair_name":
                    self._view.set_item_selectable_and_enabled(row, col)
                elif column_name == "alpha":
                    self._view.set_item_editable_and_enabled(row, col)
                elif column_name == "to_analyse":
                    self._view.set_item_checkable_and_enabled(row, col)

    def add_entry_to_table(self, row_entries, color=(255, 255, 255), tooltip=""):
        """Add an entry to the table based on row entries."""
        assert len(row_entries) == self._view.get_pairing_table.columnCount() - 1

        row_position = self._view.insert_row_in_table()
        for i, entry in enumerate(row_entries):
            item = self._view.create_table_item(entry, color, tooltip)
            column_name = pair_columns[i]

            if column_name == "pair_name":
                self._add_pair_name_entry(row_position, i, entry)
            if column_name == "group_1":
                self._add_group_selector_entry(row_position, i, entry, group="group_1")
            if column_name == "group_2":
                self._add_group_selector_entry(row_position, i, entry, group="group_2")
            elif column_name == "alpha":
                self._add_alpha_entry(row_position, i, entry)
            if column_name == "to_analyse":
                self._add_to_analyse_checkbox(item, entry)
            self._view.set_item_in_table(row_position, i, item)

        # "Guess Alpha" button in the last column
        self._add_guess_alpha_button(row_position)

    def _add_pair_name_entry(self, row, col, entry):
        """Add a validated pair name entry."""
        pair_name_widget = table_utils.ValidatedTableItem(self._view.validate_pair_name_entry)
        pair_name_widget.setText(entry)
        self._view.set_item_in_table(row, col, pair_name_widget, flags=self._view.get_default_item_flags)

    def _add_group_selector_entry(self, row, col, entry, group):
        """Add a group selector widget for group_1 or group_2."""
        group_selector_widget = self._view.group_selection_cell_widget

        if group == "group_1":
            group_selector_widget.currentIndexChanged.connect(lambda: self._view.on_cell_changed(row, 2))
        elif group == "group_2":
            group_selector_widget.currentIndexChanged.connect(lambda: self._view.on_cell_changed(row, 3))

        self._view.set_combo_box_index(group_selector_widget, entry)
        self._view.set_widget_in_table(row, col, group_selector_widget)

    def _add_alpha_entry(self, row, col, entry):
        """Add an alpha entry to the table."""
        alpha_widget = table_utils.ValidatedTableItem(self._view.validate_alpha)
        alpha_widget.setText(entry)
        self._view.set_item_in_table(row, col, alpha_widget)

    def _add_to_analyse_checkbox(self, item, entry):
        """Set the 'to_analyse' checkbox state."""
        self._view.set_checkbox_in_table(item, checked=bool(entry))

    def _add_guess_alpha_button(self, row):
        """Add the 'Guess Alpha' button to the last column."""
        guess_alpha_widget = self._view.guess_alpha_button
        guess_alpha_widget.clicked.connect(self._view.guess_alpha_clicked_from_row)
        self._view.set_widget_in_table(row, 5, guess_alpha_widget)

    # ------------------------------------------------------------------------------------------------------------------
    # Context Menu
    # ------------------------------------------------------------------------------------------------------------------
    def _context_menu_event(self, _event):
        """Overridden method for dealing with the right-click context menu"""
        menu = self._view.create_context_menu

        add_pair_action = self._context_menu_add_pair_action(self._view.add_pair_button.clicked.emit)
        remove_pair_action = self._context_menu_remove_pair_action(self._view.remove_pair_button.clicked.emit)

        if self._view.is_disabled:
            self._view.add_pair_action.setEnabled(False)
            self._view.remove_pair_action.setEnabled(False)
        # set-up the menu
        menu.addAction(add_pair_action)
        menu.addAction(remove_pair_action)
        menu.popup(self._view.cursor_position)

    def _context_menu_add_pair_action(self, slot):
        add_pair_action = self._view.get_add_pair_action
        add_pair_action.setCheckable(False)
        if len(self.get_selected_row_indices()) > 0:
            add_pair_action.setEnabled(False)
        add_pair_action.triggered.connect(slot)
        return add_pair_action

    def _context_menu_remove_pair_action(self, slot):
        if len(self.get_selected_row_indices()) > 1:
            # use plural if >1 item selected
            remove_pair_action = self._view.get_remove_pair_action(True)
        else:
            remove_pair_action = self._view.get_remove_pair_action()
        if self._view.get_pairing_table.rowCount() == 0:
            remove_pair_action.setEnabled(False)
        remove_pair_action.triggered.connect(slot)
        return remove_pair_action

    # ------------------------------------------------------------------------------------------------------------------
    # Table entry validation
    # ------------------------------------------------------------------------------------------------------------------

    def _is_edited_name_duplicated(self, new_name):
        is_name_column_being_edited = self._view.get_pairing_table.currentColumn() == 0
        is_name_unique = sum([new_name == name for name in self._model.group_and_pair_names]) == 0
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
