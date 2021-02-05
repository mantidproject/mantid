# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui

from Muon.GUI.Common.fitting_tab_widget.basic_fitting_view import BasicFittingView

from qtpy import QtWidgets

ui_general_fitting, _ = load_ui(__file__, "general_fitting.ui")


class GeneralFittingView(QtWidgets.QWidget, ui_general_fitting):
    def __init__(self, simultaneous_item_list, is_frequency_domain=False, parent=None):
        super(GeneralFittingView, self).__init__(parent)
        self.setupUi(self)

        self.basic_fitting_widget = BasicFittingView(self, is_frequency_domain)
        self.layout().addWidget(self.basic_fitting_widget)

        self._setup_simultaneous_fit_by_combo_box(simultaneous_item_list)

        self.increment_parameter_display_button.clicked.connect(self.increment_display_combo_box)
        self.decrement_parameter_display_button.clicked.connect(self.decrement_display_combo_box)

        self.disable_simul_fit_options()

        if is_frequency_domain:
            self.hide_simultaneous_fit_options()

    def update_displayed_data_combo_box(self, data_list):
        """Update the data in the parameter display combo box."""
        self.parameter_display_combo.blockSignals(True)
        name = self.parameter_display_combo.currentText()
        self.parameter_display_combo.clear()
        self.parameter_display_combo.addItems(data_list)

        index = self.parameter_display_combo.findText(name)

        if index != -1:
            self.parameter_display_combo.setCurrentIndex(index)
        else:
            self.parameter_display_combo.setCurrentIndex(0)

        self.parameter_display_combo.blockSignals(False)

    def increment_display_combo_box(self):
        """Increment the parameter display combo box."""
        index = self.parameter_display_combo.currentIndex()
        count = self.parameter_display_combo.count()

        if index < count - 1:
            self.parameter_display_combo.setCurrentIndex(index + 1)
        else:
            self.parameter_display_combo.setCurrentIndex(0)

    def decrement_display_combo_box(self):
        """Decrement the parameter display combo box."""
        index = self.parameter_display_combo.currentIndex()
        count = self.parameter_display_combo.count()

        if index != 0:
            self.parameter_display_combo.setCurrentIndex(index - 1)
        else:
            self.parameter_display_combo.setCurrentIndex(count - 1)

    def _setup_simultaneous_fit_by_combo_box(self, item_list):
        """Populate the simultaneous fit by combo box."""
        for item in item_list:
            self.simul_fit_by_combo.addItem(item)

    def set_datasets_in_function_browser(self, data_set_name_list):
        """Sets the datasets stored in the FunctionBrowser."""
        self.basic_fitting_widget.set_datasets_in_function_browser(data_set_name_list)

    def update_with_fit_outputs(self, fit_function, output_status, output_chi_squared):
        """Updates the view to show the status and results from a fit."""
        if not fit_function:
            self.basic_fitting_widget.clear_fit_status(output_chi_squared)
            return

        self.basic_fitting_widget.update_function_browser_parameters(self.is_simul_fit, fit_function)
        self.basic_fitting_widget.update_fit_status_labels(output_status, output_chi_squared)

    def update_global_fit_state(self, output_list, index):
        """Updates the global fit status label."""
        if self.is_simul_fit:
            indexed_fit = output_list[index]
            boolean_list = [indexed_fit == "success"] if indexed_fit else []
        else:
            boolean_list = [output == "success" for output in output_list if output]

        self.basic_fitting_widget.update_global_fit_status_label(boolean_list)

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
    def display_workspace(self):
        """Returns the name of the currently displayed workspace parameter."""
        return str(self.parameter_display_combo.currentText())

    @property
    def loaded_workspaces(self):
        """Returns the names of all loaded workspaces."""
        return [self.parameter_display_combo.itemText(i) for i in range(self.parameter_display_combo.count())]

    @display_workspace.setter
    def display_workspace(self, value):
        """Sets the name of the currently displayed workspace parameter."""
        index = self.parameter_display_combo.findText(value)
        if index != -1:
            self.parameter_display_combo.blockSignals(True)
            self.parameter_display_combo.setCurrentIndex(index)
            self.parameter_display_combo.blockSignals(False)

    @property
    def fit_object(self):
        """Returns the global fitting function."""
        return self.basic_fitting_widget.getGlobalFunction()

    @property
    def minimizer(self):
        """Returns the selected minimizer."""
        return self.basic_fitting_widget.minimizer()

    @property
    def start_time(self):
        """Returns the selected start X."""
        return self.basic_fitting_widget.start_time()

    @start_time.setter
    def start_time(self, value):
        """Sets the selected start X."""
        self.basic_fitting_widget.start_time(value)

    @property
    def end_time(self):
        """Returns the selected end X."""
        return self.basic_fitting_widget.end_time()

    @end_time.setter
    def end_time(self, value):
        """Sets the selected end X."""
        self.basic_fitting_widget.end_time(value)

    @property
    def evaluation_type(self):
        """Returns the selected evaluation type."""
        return self.basic_fitting_widget.evaluation_type()

    @property
    def simultaneous_fit_by(self):
        """Returns what you are simultaneously fitting by (Run or Group/Pair)."""
        return self.simul_fit_by_combo.currentText()

    @simultaneous_fit_by.setter
    def simultaneous_fit_by(self, fit_by_text):
        """Sets what you are simultaneously fitting by (Run or Group/Pair)."""
        index = self.simul_fit_by_combo.findText(fit_by_text)
        if index != -1:
            self.simul_fit_by_combo.blockSignals(True)
            self.simul_fit_by_combo.setCurrentIndex(index)
            self.simul_fit_by_combo.blockSignals(False)

    @property
    def simultaneous_fit_by_specifier(self):
        """Returns the run, group or pair name."""
        return self.simul_fit_by_specifier.currentText()

    @property
    def fit_to_raw(self):
        """Returns whether or not fitting to raw data is ticked."""
        return self.basic_fitting_widget.fit_to_raw()

    @fit_to_raw.setter
    def fit_to_raw(self, value):
        """Sets whether or not you are fitting to raw data."""
        self.basic_fitting_widget.fit_to_raw(value)

    @property
    def plot_guess(self):
        """Returns true if plot guess is ticked."""
        return self.basic_fitting_widget.plot_guess()

    @plot_guess.setter
    def plot_guess(self, value):
        """Sets whether or not plot guess is ticked."""
        self.basic_fitting_widget.plot_guess(value)

    @property
    def function_name(self):
        """Returns the function name being used."""
        return self.basic_fitting_widget.function_name()

    @function_name.setter
    def function_name(self, function_name):
        """Sets the function name being used."""
        self.basic_fitting_widget.function_name(function_name)

    def warning_popup(self, message):
        """Displays a warning message."""
        self.basic_fitting_widget.warning_popup(message)

    def get_index_for_start_end_times(self):
        """Returns the index of the currently displayed workspace."""
        current_index = self.parameter_display_combo.currentIndex()
        return current_index if current_index != -1 else 0

    def get_global_parameters(self):
        """Returns a list of global parameters."""
        return self.basic_fitting_widget.get_global_parameters()

    def switch_to_simultaneous(self):
        """Switches the view to simultaneous mode."""
        self.basic_fitting_widget.switch_to_simultaneous()

    def switch_to_single(self):
        """Switches the view to single mode."""
        self.basic_fitting_widget.switch_to_single()

    def disable_simul_fit_options(self):
        """Disables the simultaneous fit options."""
        self.simul_fit_by_combo.setEnabled(False)
        self.simul_fit_by_specifier.setEnabled(False)

    def hide_simultaneous_fit_options(self):
        """Hides the simultaneous fit options."""
        self.simul_fit_checkbox.hide()
        self.simul_fit_by_combo.hide()
        self.simul_fit_by_specifier.hide()

    def enable_simul_fit_options(self):
        """Enables the simultaneous fit options."""
        self.simul_fit_by_combo.setEnabled(True)
        self.simul_fit_by_specifier.setEnabled(True)

    @property
    def is_simul_fit(self):
        """Returns true if in simultaneous mode."""
        return self.simul_fit_checkbox.isChecked()

    @is_simul_fit.setter
    def is_simul_fit(self, simultaneous):
        """Sets whether or not you are in simultaneous mode."""
        self.simul_fit_checkbox.blockSignals(True)
        self.simul_fit_checkbox.setChecked(simultaneous)
        self.simul_fit_checkbox.blockSignals(False)

    def setup_fit_by_specifier(self, choices):
        """Setup the fit by specifier combo box."""
        self.simul_fit_by_specifier.blockSignals(True)
        self.simul_fit_by_specifier.clear()
        self.simul_fit_by_specifier.addItems(choices)
        self.simul_fit_by_specifier.blockSignals(False)
        self.simul_fit_by_specifier.currentIndexChanged.emit(0)
