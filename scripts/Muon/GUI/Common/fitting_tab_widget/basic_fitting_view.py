# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui

from Muon.GUI.Common.fitting_tab_widget.fit_controls_view import FitControlsView
from Muon.GUI.Common.fitting_tab_widget.fit_function_options_view import FitFunctionOptionsView
from Muon.GUI.Common.message_box import warning

from qtpy import QtWidgets

ui_fitting_layout, _ = load_ui(__file__, "fitting_layout.ui")


class BasicFittingView(QtWidgets.QWidget, ui_fitting_layout):

    def __init__(self, parent=None, is_frequency_domain=False):
        super(BasicFittingView, self).__init__(parent)
        self.setupUi(self)

        self.fit_controls = FitControlsView(self)
        self.fit_function_options = FitFunctionOptionsView(self, is_frequency_domain)

        self.fit_controls_layout.addWidget(self.fit_controls)
        self.fit_function_options_layout.addWidget(self.fit_function_options)

    def set_datasets_in_function_browser(self, data_set_name_list):
        """Sets the datasets stored in the FunctionBrowser."""
        self.fit_function_options.set_datasets_in_function_browser(data_set_name_list)

    def set_current_dataset_index(self, dataset_index):
        """Sets the index of the current dataset."""
        self.fit_function_options.set_current_dataset_index(dataset_index)

    def update_with_fit_outputs(self, fit_function, output_status, output_chi_squared):
        """Updates the view to show the status and results from a fit."""
        if not fit_function:
            self.fit_controls.clear_fit_status(output_chi_squared)
            return

        self.fit_function_options.update_function_browser_parameters(False, fit_function)
        self.fit_controls.update_fit_status_labels(output_status, output_chi_squared)

    def update_global_fit_state(self, output_list):
        """Updates the global fit status label."""
        self.fit_controls.update_global_fit_status_label([output == "success" for output in output_list if output])

    # def set_slot_for_fit_generator_clicked(self, slot):
    #     self.fit_generator_button.clicked.connect(slot)
    #
    # def set_slot_for_display_workspace_changed(self, slot):
    #     self.parameter_display_combo.currentIndexChanged.connect(slot)
    #
    # def set_slot_for_use_raw_changed(self, slot):
    #     self.fit_to_raw_data_checkbox.stateChanged.connect(slot)
    #
    # def set_slot_for_fit_type_changed(self, slot):
    #     self.simul_fit_checkbox.toggled.connect(slot)
    #
    # def set_slot_for_fit_button_clicked(self, slot):
    #     self.fit_button.clicked.connect(slot)
    #
    # def set_slot_for_start_x_updated(self, slot):
    #     self.time_start.editingFinished.connect(slot)
    #
    # def set_slot_for_end_x_updated(self, slot):
    #     self.time_end.editingFinished.connect(slot)
    #
    # def set_slot_for_simul_fit_by_changed(self, slot):
    #     self.simul_fit_by_combo.currentIndexChanged.connect(slot)
    #
    # def set_slot_for_simul_fit_specifier_changed(self, slot):
    #     self.simul_fit_by_specifier.currentIndexChanged.connect(slot)
    #
    # def set_slot_for_minimiser_changed(self, slot):
    #     self.minimizer_combo.currentIndexChanged.connect(slot)
    #
    # def set_slot_for_evaluation_type_changed(self, slot):
    #     self.evaluation_combo.currentIndexChanged.connect(slot)

    @property
    def fit_object(self):
        """Returns the global fitting function."""
        return self.fit_function_options.fit_object

    @property
    def minimizer(self):
        """Returns the selected minimizer."""
        return self.fit_function_options.minimizer

    @property
    def start_time(self):
        """Returns the selected start X."""
        return self.fit_function_options.start_time

    @start_time.setter
    def start_time(self, value):
        """Sets the selected start X."""
        self.fit_function_options.start_time(value)

    @property
    def end_time(self):
        """Returns the selected end X."""
        return self.fit_function_options.end_time

    @end_time.setter
    def end_time(self, value):
        """Sets the selected end X."""
        self.fit_function_options.end_time(value)

    @property
    def evaluation_type(self):
        """Returns the selected evaluation type."""
        return self.fit_function_options.evaluation_type

    @property
    def fit_to_raw(self):
        """Returns whether or not fitting to raw data is ticked."""
        return self.fit_function_options.fit_to_raw

    @fit_to_raw.setter
    def fit_to_raw(self, value):
        """Sets whether or not you are fitting to raw data."""
        self.fit_function_options.fit_to_raw(value)

    @property
    def plot_guess(self):
        """Returns true if plot guess is ticked."""
        return self.fit_controls.plot_guess

    @plot_guess.setter
    def plot_guess(self, value):
        """Sets whether or not plot guess is ticked."""
        self.fit_controls.plot_guess(value)

    def enable_undo_fit(self, enable):
        """Sets whether or not undo fit is enabled."""
        self.fit_controls.enable_undo_fit(enable)

    @property
    def function_name(self):
        """Returns the function name being used."""
        return self.fit_function_options.function_name

    @function_name.setter
    def function_name(self, function_name):
        """Sets the function name being used."""
        self.fit_function_options.function_name(function_name)

    def warning_popup(self, message):
        """Displays a warning message."""
        warning(message, parent=self)

    def get_global_parameters(self):
        """Returns a list of global parameters."""
        return self.fit_function_options.get_global_parameters()

    def switch_to_simultaneous(self):
        """Switches the view to simultaneous mode."""
        self.fit_function_options.switch_to_simultaneous()

    def switch_to_single(self):
        """Switches the view to single mode."""
        self.fit_function_options.switch_to_single()
