# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.fitting_tab_widget.basic_fitting_view import BasicFittingView
from Muon.GUI.Common.fitting_tab_widget.general_fitting_options_view import GeneralFittingOptionsView

SINGLE_FIT_LABEL = "Select Workspace"
SIMULTANEOUS_FIT_LABEL = "Display parameters for"


class GeneralFittingView(BasicFittingView):

    def __init__(self, parent=None, is_frequency_domain=False):
        super(GeneralFittingView, self).__init__(parent)

        self.general_fitting_options = GeneralFittingOptionsView(self, is_frequency_domain)
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

    def update_global_fit_status(self, fit_statuses, index):
        """Updates the global fit status label."""
        if self.is_simultaneous_fit_ticked:
            indexed_fit = fit_statuses[index]
            self.fit_controls.update_global_fit_status_label([indexed_fit == "success"] if indexed_fit else [])
        else:
            super().update_global_fit_status(fit_statuses, index)

    def update_fit_function(self, fit_function):
        """Updates the parameters of a fit function shown in the view."""
        self.fit_function_options.update_function_browser_parameters(self.is_simultaneous_fit_ticked, fit_function)

    @property
    def display_workspace(self):
        """Returns the name of the currently displayed workspace parameter."""
        return self.general_fitting_options.display_workspace

    @display_workspace.setter
    def display_workspace(self, value):
        """Sets the name of the currently displayed workspace parameter."""
        self.general_fitting_options.display_workspace = value

    def number_of_domains(self):
        return self.general_fitting_options.number_of_domains()

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

    @property
    def current_dataset_index(self):
        """Returns the index of the currently displayed dataset."""
        return self.general_fitting_options.current_dataset_index

    def switch_to_simultaneous(self):
        """Switches the view to simultaneous fit mode."""
        super().switch_to_simultaneous()
        self.set_workspace_combo_box_label(SIMULTANEOUS_FIT_LABEL)
        self.enable_simultaneous_fit_options()

    def switch_to_single(self):
        """Switches the view to single fit mode."""
        super().switch_to_single()
        self.set_workspace_combo_box_label(SINGLE_FIT_LABEL)
        self.disable_simultaneous_fit_options()

    def hide_simultaneous_fit_options(self):
        """Hides the simultaneous fit options."""
        self.general_fitting_options.hide_simultaneous_fit_options()

    def disable_simultaneous_fit_options(self):
        """Disables the simultaneous fit options."""
        self.general_fitting_options.disable_simultaneous_fit_options()

    def enable_simultaneous_fit_options(self):
        """Enables the simultaneous fit options."""
        self.general_fitting_options.enable_simultaneous_fit_options()

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
