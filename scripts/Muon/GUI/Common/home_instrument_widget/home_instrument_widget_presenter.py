# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget


class InstrumentWidgetPresenter(HomeTabSubWidget):

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

        self._view.on_rebin_type_changed(self.handle_rebin_type_changed)

        self._view.on_instrument_changed(self.handle_instrument_changed)

        self._view.on_double_pulse_time_changed(self.handle_double_pulse_time_changed)
        self._view.on_double_pulse_checkState_changed(self.handle_double_pulse_enabled)

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

        if self._view.last_good_data_state():
            last_good_data = self._model.get_file_last_good_data()
        else:
            last_good_data = self._model.get_last_good_data()
        self._view.set_last_good_data(last_good_data)

        if self._view.time_zero_state():
            time_zero = self._model.get_file_time_zero()
            self._view.set_time_zero(time_zero)
        else:
            time_zero = self._model.get_user_time_zero()
            self._view.set_time_zero(time_zero)

        self._view.set_instrument(self._model._data.instrument)

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
            last_good_data = self._model.get_file_last_good_data()
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

    def handle_double_pulse_time_changed(self):
        double_pulse_time = self._view.get_double_pulse_time()
        self._model.set_double_pulse_time(double_pulse_time)

    def handle_double_pulse_enabled(self):
        pulseType = self._view.double_pulse_state()
        enabled = pulseType == 'Double Pulse'
        self._view.double_pulse_edit_enabled(enabled)
        self._model.set_double_pulse_enabled(enabled)
