# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import re
from contextlib import contextmanager
from mantidqtinterfaces.Muon.GUI.Common.muon_pair import MuonPair
from mantidqtinterfaces.Muon.GUI.Common.utilities.run_string_utils import valid_name_regex, valid_alpha_regex
from mantidqtinterfaces.Muon.GUI.Common.utilities.general_utils import round_value
from mantidqt.utils.observer_pattern import GenericObservable
from mantidqtinterfaces.Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import RowValid
from mantidqtinterfaces.Muon.GUI.Common.grouping_table_widget.grouping_table_widget_presenter import row_colors, row_tooltips
from mantidqtinterfaces.Muon.GUI.Common.pairing_table_widget.pairing_table_widget_view import get_pair_columns, pair_columns


class PairingTablePresenter(object):
    def __init__(self, view, model):
        self._view = view
        self._model = model

        self._view.subscribe(self)
        self._view.subscribe_notifiers_to_presenter()

        self.selected_pair_changed_notifier = GenericObservable()

        self._dataChangedNotifier = lambda: 0

        # notify if Guess Alpha clicked for any table entries
        self.guessAlphaNotifier = GenericObservable()

    def show(self):
        self._view.show()

    def on_data_changed(self, notifier):
        self._dataChangedNotifier = notifier

    def notify_data_changed(self):
        self._dataChangedNotifier()

    def disable_editing(self):
        """Disables editing mode."""
        self.enable_updates()
        self._view.set_is_disabled(True)
        self._view.disable_all_buttons()
        self._disable_all_table_items()
        self.disable_updates()

    def enable_editing(self):
        """Enables editing mode."""
        self.enable_updates()
        self._view.set_is_disabled(False)
        self._view.enable_all_buttons()
        self._enable_all_table_items()
        self.disable_updates()

    @contextmanager
    def disable_updates_context(self):
        """Context manager to disable updates."""
        self.enable_updates()
        try:
            yield
        finally:
            self.disable_updates()

    def enable_updates(self):
        """Prevent update signals being sent."""
        self._view.set_is_updating(True)

    def disable_updates(self):
        """Allow update signals to be sent."""
        self._view.set_is_updating(False)

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
        match get_pair_columns()[col]:
            case "pair_name" if not self.validate_pair_name(changed_item_text):
                update_model = False
            case "group_1":
                if changed_item_text == self.get_table_item_text(row, get_pair_columns().index("group_2")):
                    table[row][get_pair_columns().index("group_2")] = self._model.pairs[row].forward_group
            case "group_2":
                if changed_item_text == self.get_table_item_text(row, get_pair_columns().index("group_1")):
                    table[row][get_pair_columns().index("group_1")] = self._model.pairs[row].backward_group
            case "alpha":
                if not self.validate_alpha(changed_item_text):
                    update_model = False
                else:
                    rounded_item = round_value(changed_item_text, self._model.get_context.group_pair_context.alpha_precision)
                    table[row][col] = rounded_item
            case "to_analyse":
                update_model = False
                self.to_analyse_data_checkbox_changed(changed_item.checkState(), row, pair_name)
            case _:
                pass

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
        with self.disable_updates_context():
            self.clear()
            for pair in self._model.pairs:
                if isinstance(pair, MuonPair):
                    to_analyse = True if pair.name in self._model.selected_pairs else False
                    forward_group_periods = self._model.get_context.group_pair_context[pair.forward_group].periods
                    backward_group_periods = self._model.get_context.group_pair_context[pair.backward_group].periods
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
        with self.disable_updates_context():
            self.update_group_selections()
            assert isinstance(pair, MuonPair)
            entry = [str(pair.name), to_analyse, str(pair.forward_group), str(pair.backward_group), str(pair.alpha)]
            self.add_entry_to_table(entry, color, tool_tip)

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

        match column_name:
            case "group_1" | "group_2":
                return self._view.get_widget_current_text(row, col)
            case "guess_alpha":
                return "Guess"
            case _:
                return self._view.get_item_text(row, col)

    def get_table_contents(self):
        if self._view.is_updating:
            return []

        ret = [[None for _ in range(self.num_cols())] for _ in range(self.num_rows())]
        for row in range(self.num_rows()):
            for col in range(self.num_cols()):
                column_name = pair_columns[col]

                match column_name:
                    case "group_1" | "group_2":
                        ret[row][col] = self._view.get_widget_current_text(row, col)
                    case "guess_alpha":
                        ret[row][col] = "Guess"
                    case _:
                        ret[row][col] = self._view.get_item_text(row, col)

        return ret

    # ------------------------------------------------------------------------------------------------------------------
    # Enabling / Disabling the table
    # ------------------------------------------------------------------------------------------------------------------
    def _set_table_items_enabled(self, enable=True):
        for row in range(self.num_rows()):
            for col in range(self.num_cols()):
                column_name = pair_columns[col]

                if column_name in ["group_1", "group_2", "guess_alpha"]:
                    self._view.set_widget_enabled(row, col, enable)
                elif enable:
                    # Actions for enabling items only
                    match column_name:
                        case "pair_name":
                            self._view.set_item_selectable_and_enabled(row, col)
                        case "alpha":
                            self._view.set_item_editable_and_enabled(row, col)
                        case "to_analyse":
                            self._view.set_item_checkable_and_enabled(row, col)
                else:
                    # Actions only for disabling items
                    match column_name:
                        case _:
                            self._view.set_item_selectable(row, col)

    def _disable_all_table_items(self):
        self._set_table_items_enabled(enable=False)

    def _enable_all_table_items(self):
        self._set_table_items_enabled(enable=True)

    def add_entry_to_table(self, row_entries, color=(255, 255, 255), tooltip=""):
        """Add an entry to the table based on row entries."""
        assert len(row_entries) == self._view.get_pairing_table.columnCount() - 1

        row_position = self._view.insert_row_in_table()
        for i, entry in enumerate(row_entries):
            item = self._view.create_table_item(entry, color, tooltip)
            column_name = pair_columns[i]

            match column_name:
                case "pair_name":
                    self._view.add_pair_name_entry(row_position, i, entry)
                case "group_1":
                    self._view.add_group_selector_entry(row_position, i, entry, group="group_1")
                case "group_2":
                    self._view.add_group_selector_entry(row_position, i, entry, group="group_2")
                case "alpha":
                    self._view.add_alpha_entry(row_position, i, entry)
                case "to_analyse":
                    self._view.add_to_analyse_checkbox(item, entry)

            self._view.set_item_in_table(row_position, i, item)

        # "Guess Alpha" button in the last column
        self._view.add_guess_alpha_button(row_position)

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
