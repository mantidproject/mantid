# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.fitting_tab_widget.basic_fitting_view import BasicFittingView
from Muon.GUI.Common.fitting_tab_widget.general_fitting_options_view import GeneralFittingOptionsView


class GeneralFittingView(BasicFittingView):

    def __init__(self, parent=None, is_frequency_domain=False, simultaneous_item_list=[]):
        super(GeneralFittingView, self).__init__(parent)
        self.setupUi(self)

        self.general_fitting_options = GeneralFittingOptionsView(self, is_frequency_domain, simultaneous_item_list)
        self.general_fitting_options_layout.addWidget(self.general_fitting_options)

    def update_displayed_data_combo_box(self, data_list):
        """Update the data in the parameter display combo box."""
        self.general_fitting_options.update_displayed_data_combo_box(data_list)

    def update_with_fit_outputs(self, fit_function, output_status, output_chi_squared):
        """Updates the view to show the status and results from a fit."""
        if not fit_function:
            self.fit_controls.clear_fit_status(output_chi_squared)
            return

        self.fit_function_options.update_function_browser_parameters(self.is_simul_fit, fit_function)
        self.fit_controls.update_fit_status_labels(output_status, output_chi_squared)

    def update_global_fit_state(self, output_list, index):
        """Updates the global fit status label."""
        if self.is_simul_fit:
            indexed_fit = output_list[index]
            boolean_list = [indexed_fit == "success"] if indexed_fit else []
        else:
            boolean_list = [output == "success" for output in output_list if output]

        self.fit_controls.update_global_fit_status_label(boolean_list)

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
        return self.general_fitting_options.display_workspace

    @property
    def loaded_workspaces(self):
        """Returns the names of all loaded workspaces."""
        return self.general_fitting_options.loaded_workspaces

    @display_workspace.setter
    def display_workspace(self, value):
        """Sets the name of the currently displayed workspace parameter."""
        self.general_fitting_options.display_workspace(value)

    @property
    def simultaneous_fit_by(self):
        """Returns what you are simultaneously fitting by (Run or Group/Pair)."""
        return self.general_fitting_options.simultaneous_fit_by

    @simultaneous_fit_by.setter
    def simultaneous_fit_by(self, fit_by_text):
        """Sets what you are simultaneously fitting by (Run or Group/Pair)."""
        self.general_fitting_options.simultaneous_fit_by(fit_by_text)

    @property
    def simultaneous_fit_by_specifier(self):
        """Returns the run, group or pair name."""
        return self.general_fitting_options.simultaneous_fit_by_specifier

    def get_index_for_start_end_times(self):
        """Returns the index of the currently displayed workspace."""
        return self.general_fitting_options.get_index_for_start_end_times()

    def disable_simul_fit_options(self):
        """Disables the simultaneous fit options."""
        self.general_fitting_options.disable_simul_fit_options()

    def hide_simultaneous_fit_options(self):
        """Hides the simultaneous fit options."""
        self.general_fitting_options.hide_simultaneous_fit_options()

    def enable_simul_fit_options(self):
        """Enables the simultaneous fit options."""
        self.general_fitting_options.enable_simul_fit_options()

    @property
    def is_simul_fit(self):
        """Returns true if in simultaneous mode."""
        return self.general_fitting_options.is_simul_fit

    @is_simul_fit.setter
    def is_simul_fit(self, simultaneous):
        """Sets whether or not you are in simultaneous mode."""
        self.general_fitting_options.is_simul_fit(simultaneous)

    def setup_fit_by_specifier(self, choices):
        """Setup the fit by specifier combo box."""
        self.general_fitting_options.setup_fit_by_specifier(choices)
