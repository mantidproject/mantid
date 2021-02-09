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

        self.general_fitting_options = GeneralFittingOptionsView(self, is_frequency_domain, simultaneous_item_list)
        self.general_fitting_options_layout.addWidget(self.general_fitting_options)

    def set_slot_for_display_workspace_changed(self, slot):
        """Connect the slot for the display workspace combo box being changed."""
        self.general_fitting_options.set_slot_for_display_workspace_changed(slot)

    def set_slot_for_fitting_mode_changed(self, slot):
        """Connect the slot for the simultaneous fit check box."""
        self.general_fitting_options.set_slot_for_fitting_mode_changed(slot)

    def set_slot_for_simultaneous_fit_by_changed(self, slot):
        """Connect the slot for the fit by combo box being changed."""
        self.general_fitting_options.set_slot_for_simultaneous_fit_by_changed(slot)

    def set_slot_for_simultaneous_fit_by_specifier_changed(self, slot):
        """Connect the slot for the fit specifier combo box being changed."""
        self.general_fitting_options.set_slot_for_simultaneous_fit_by_specifier_changed(slot)

    def update_displayed_data_combo_box(self, data_list):
        """Update the data in the parameter display combo box."""
        self.general_fitting_options.update_displayed_data_combo_box(data_list)

    def update_with_fit_outputs(self, fit_function, output_status, output_chi_squared):
        """Updates the view to show the status and results from a fit."""
        if not fit_function:
            self.fit_controls.clear_fit_status(output_chi_squared)
            return

        self.fit_function_options.update_function_browser_parameters(self.is_simultaneous_fit_ticked, fit_function)
        self.fit_controls.update_fit_status_labels(output_status, output_chi_squared)

    def update_global_fit_state(self, output_list, index):
        """Updates the global fit status label."""
        if self.is_simultaneous_fit_ticked:
            indexed_fit = output_list[index]
            boolean_list = [indexed_fit == "success"] if indexed_fit else []
        else:
            boolean_list = [output == "success" for output in output_list if output]

        self.fit_controls.update_global_fit_status_label(boolean_list)

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
        self.general_fitting_options.display_workspace = value

    @property
    def simultaneous_fit_by(self):
        """Returns what you are simultaneously fitting by (Run or Group/Pair)."""
        return self.general_fitting_options.simultaneous_fit_by

    @simultaneous_fit_by.setter
    def simultaneous_fit_by(self, fit_by_text):
        """Sets what you are simultaneously fitting by (Run or Group/Pair)."""
        self.general_fitting_options.simultaneous_fit_by = fit_by_text

    @property
    def simultaneous_fit_by_specifier(self):
        """Returns the run, group or pair name."""
        return self.general_fitting_options.simultaneous_fit_by_specifier

    def get_index_for_start_end_times(self):
        """Returns the index of the currently displayed workspace."""
        return self.general_fitting_options.get_index_for_start_end_times()

    def hide_simultaneous_fit_options(self):
        """Hides the simultaneous fit options."""
        self.general_fitting_options.hide_simultaneous_fit_options()

    def disable_simultaneous_fit_options(self):
        """Disables the simultaneous fit options."""
        self.general_fitting_options.disable_simultaneous_fit_options()

    def enable_simultaneous_fit_options(self):
        """Enables the simultaneous fit options."""
        self.general_fitting_options.enable_simultaneous_fit_options()

    def enable_simultaneous_fit_by_specifier(self, enable):
        """Enables or disables the simultaneous fit by combo box."""
        self.general_fitting_options.enable_simultaneous_fit_by_specifier(enable)

    @property
    def is_simultaneous_fit_ticked(self):
        """Returns true if in simultaneous mode."""
        return self.general_fitting_options.is_simultaneous_fit_ticked

    @is_simultaneous_fit_ticked.setter
    def is_simultaneous_fit_ticked(self, simultaneous):
        """Sets whether or not you are in simultaneous mode."""
        self.general_fitting_options.is_simultaneous_fit_ticked(simultaneous)

    def setup_fit_by_specifier(self, choices):
        """Setup the fit by specifier combo box."""
        self.general_fitting_options.setup_fit_by_specifier(choices)

    def set_workspace_combo_box_label(self, text):
        """Sets the text in the workspace combo box label."""
        self.general_fitting_options.set_workspace_combo_box_label(text)
