# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget
import Muon.GUI.Common.utilities.load_utils as load_utils
from Muon.GUI.Common.utilities.muon_file_utils import filter_for_extensions


class InstrumentWidgetPresenter(HomeTabSubWidget):

    @staticmethod
    def dead_time_from_data_text(dead_times):
        mean = sum(dead_times) / len(dead_times)
        label = "From {0:.3f} to {1:.3f} (ave. {2:.3f})".format(min(dead_times), max(dead_times), mean)
        return label

    def __init__(self, view, model):
        self._view = view
        self._model = model

        self._view.on_time_zero_checkState_changed(self.handle_loaded_time_zero_checkState_change)
        self._view.on_time_zero_changed(self.handle_user_changes_time_zero)

        self._view.on_first_good_data_checkState_changed(self.handle_loaded_first_good_data_checkState_change)
        self._view.on_first_good_data_changed(self.handle_user_changes_first_good_data)

        self._view.on_last_good_data_checkState_changed(self.handle_loaded_last_good_data_checkState_change)
        self._view.on_last_good_data_changed(self.handle_user_changes_last_good_data)

        self._view.on_fixed_rebin_edit_changed(self.handle_fixed_rebin_changed)
        self._view.on_variable_rebin_edit_changed(self.handle_variable_rebin_changed)

        self._view.on_dead_time_from_data_selected(self.handle_user_selects_dead_time_from_data)
        self._view.on_dead_time_unselected(self.handle_dead_time_unselected)
        self._view.on_dead_time_browse_clicked(self.handle_dead_time_browse_clicked)
        self._view.on_dead_time_from_file_selected(self.handle_dead_time_from_table_workspace_selected)
        self._view.on_dead_time_file_option_changed(self.handle_dead_time_table_workspace_selector_changed)

        self._view.on_rebin_type_changed(self.handle_rebin_type_changed)

        self._view.on_instrument_changed(self.handle_instrument_changed)

        self.handle_loaded_time_zero_checkState_change()
        self.handle_loaded_first_good_data_checkState_change()
        self.handle_loaded_last_good_data_checkState_change()

    def show(self):
        self._view.show()

    def update_view_from_model(self):
        if self._view.first_good_data_state():
            first_good_data = self._model.get_file_first_good_data()
            self._view.set_first_good_data(first_good_data)
        else:
            first_good_data = self._model.get_user_first_good_data()
            self._view.set_first_good_data(first_good_data)

        last_good_data = self._model.get_last_good_data()
        self._view.set_last_good_data(last_good_data)

        if self._view.time_zero_state():
            time_zero = self._model.get_file_time_zero()
            self._view.set_time_zero(time_zero)
        else:
            time_zero = self._model.get_user_time_zero()
            self._view.set_time_zero(time_zero)

        self._view.set_instrument(self._model._data.instrument)
        self.handle_user_selects_dead_time_from_data()

    def clear_view(self):
        self._view.set_time_zero(0.0)
        self._view.set_first_good_data(0.0)
        self._view.set_last_good_data(0.0)
        self._view.set_combo_boxes_to_default()
        self._view.set_checkboxes_to_defualt()

    # ------------------------------------------------------------------------------------------------------------------
    # Time Zero
    # ------------------------------------------------------------------------------------------------------------------

    def handle_user_changes_time_zero(self):
        time_zero = self._view.get_time_zero()
        self._model.set_user_time_zero(time_zero)

    def handle_loaded_time_zero_checkState_change(self):
        if self._view.time_zero_state():
            self._model.set_time_zero_from_file(True)
            time_zero = self._model.get_file_time_zero()
            self._view.set_time_zero(time_zero)
        else:
            self._model.set_time_zero_from_file(False)
            time_zero = self._model.get_user_time_zero()
            self._view.set_time_zero(time_zero)

    # ------------------------------------------------------------------------------------------------------------------
    # First Good Data
    # ------------------------------------------------------------------------------------------------------------------

    def handle_user_changes_first_good_data(self):
        first_good_data = self._view.get_first_good_data()
        self._model.set_user_first_good_data(first_good_data)

    def handle_loaded_first_good_data_checkState_change(self):
        if self._view.first_good_data_state():
            self._model.set_first_good_data_source(True)
            first_good_data = self._model.get_file_first_good_data()
            self._view.set_first_good_data(first_good_data)
        else:
            self._model.set_first_good_data_source(False)
            first_good_data = self._model.get_user_first_good_data()
            self._view.set_first_good_data(first_good_data)

    def handle_user_changes_last_good_data(self):
        last_good_data = self._view.get_last_good_data()
        self._model.set_user_last_good_data(last_good_data)

    def handle_loaded_last_good_data_checkState_change(self):
        if self._view.last_good_data_state():
            self._model.set_last_good_data_source(True)
            last_good_data = self._model.get_last_good_data()
            self._view.set_last_good_data(last_good_data)
        else:
            self._model.set_last_good_data_source(False)
            last_good_data = self._model.get_last_good_data()
            self._view.set_last_good_data(last_good_data)

    # ------------------------------------------------------------------------------------------------------------------
    # Rebin
    # ------------------------------------------------------------------------------------------------------------------

    def handle_fixed_rebin_changed(self):
        fixed_bin_size = self._view.get_fixed_bin_text()
        self._model.add_fixed_binning(fixed_bin_size)

    def handle_variable_rebin_changed(self):
        variable_bin_size = self._view.get_variable_bin_text()
        valid, message = self._model.validate_variable_rebin_string(variable_bin_size)
        if not valid:
            self._view.rebin_variable_edit.setText(self._model.get_variable_binning())
            self._view.warning_popup(message)
        else:
            self._model.add_variable_binning(variable_bin_size)

    def handle_rebin_type_changed(self):
        rebin_type = self._view.rebin_selector.currentText()
        self._model.update_binning_type(rebin_type)

    # ------------------------------------------------------------------------------------------------------------------
    # Instrument
    # ------------------------------------------------------------------------------------------------------------------

    def handle_instrument_changed(self):
        """User changes the selected instrument."""
        instrument = self._view.get_instrument()
        if instrument != self._model._data.instrument:
            self._model._data.instrument = instrument
            self._view.set_instrument(instrument, block=True)

    # ------------------------------------------------------------------------------------------------------------------
    # Dead Time
    # ------------------------------------------------------------------------------------------------------------------

    def handle_dead_time_browse_clicked(self):
        """User selects the option to Browse for a nexus file to load dead times from."""
        filename = self._view.show_file_browser_and_return_selection(
            filter_for_extensions(['nxs']), [''], multiple_files=False)[0]

        if filename == '':
            return

        name = load_utils.load_dead_time_from_filename(filename)
        if name == "":
            self._view.warning_popup("File does not appear to contain dead time data.")
            return

        # switch the view to the "from table workspace" option
        self._view.set_dead_time_selection(1)
        is_set = self._view.set_dead_time_file_selection_text(name)
        if not is_set:
            self._view.warning_popup("Dead time table cannot be loaded")

    def handle_user_selects_dead_time_from_data(self):
        """User chooses to load dead time from the currently loaded workspace."""
        dtc = self._model.get_dead_time_table_from_data()
        if dtc is not None:
            self._model.set_dead_time_from_data()
            dead_times = dtc.toDict()['dead-time']
            dead_time_text = self.dead_time_from_data_text(dead_times)
            self._view.set_dead_time_label(dead_time_text)
            if self._view.dead_time_selector.currentIndex() == 0:
                self._view.dead_time_label_3.setVisible(True)
            else:
                self._view.dead_time_label_3.hide()
        else:
            self._view.set_dead_time_label("No loaded dead time")

    def set_dead_time_text_to_default(self):
        """by default the dead time text should onl contain 0.0."""
        dead_time_text = self.dead_time_from_data_text([0.0])
        self._view.set_dead_time_label(dead_time_text)

    def handle_dead_time_from_table_workspace_selected(self):
        """User has selected the dead time "from Table Workspace" option."""
        table_names = load_utils.get_table_workspace_names_from_ADS()
        self._view.populate_dead_time_combo(table_names)
        self.set_dead_time_text_to_default()

    def handle_dead_time_unselected(self):
        """User has set dead time combo to 'None'."""
        self.set_dead_time_text_to_default()
        self._model.set_dead_time_to_none()

    def handle_dead_time_table_workspace_selector_changed(self):
        """The user changes the selected Table Workspace to use as dead time."""
        selection = self._view.get_dead_time_file_selection()
        if selection == "None" or selection == "":
            self.handle_dead_time_unselected()
            return
        try:
            self._model.check_dead_time_file_selection(selection)
            self._model.set_user_dead_time_from_ADS(selection)
            dead_times = self._model._context.gui_context['DeadTimeTable'].toDict()['dead-time']
            dead_time_text = self.dead_time_from_data_text(dead_times)
            self._view.set_dead_time_label(dead_time_text)
        except ValueError as error:
            self._handle_selected_table_is_invalid()
            self._view.warning_popup(error.args[0])

    def _handle_selected_table_is_invalid(self):
        self._model.set_dead_time_to_none()
        self._view.set_dead_time_file_selection(0)
        self.set_dead_time_text_to_default()
