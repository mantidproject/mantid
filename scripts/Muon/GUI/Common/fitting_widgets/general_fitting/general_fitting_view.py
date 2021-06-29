# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import IFunction

from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_view import BasicFittingView
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_options_view import GeneralFittingOptionsView

from qtpy.QtWidgets import QWidget


class GeneralFittingView(BasicFittingView):
    """
    The GeneralFittingView derives from the BasicFittingView. It adds the GeneralFittingOptionsView to the widget.
    """

    def __init__(self, parent: QWidget = None, is_frequency_domain: bool = False):
        """Initializes the GeneralFittingView, and adds the GeneralFittingOptionsView widget."""
        super(GeneralFittingView, self).__init__(parent)

        self.general_fitting_options = GeneralFittingOptionsView(self, is_frequency_domain)
        self.general_fitting_options_layout.addWidget(self.general_fitting_options)

    def set_slot_for_dataset_changed(self, slot) -> None:
        """Connect the slot for the display workspace combo box being changed."""
        self.general_fitting_options.set_slot_for_dataset_changed(slot)

    def set_slot_for_fitting_mode_changed(self, slot) -> None:
        """Connect the slot for the simultaneous fit check box."""
        self.general_fitting_options.set_slot_for_fitting_mode_changed(slot)

    def set_slot_for_simultaneous_fit_by_changed(self, slot) -> None:
        """Connect the slot for the fit by combo box being changed."""
        self.general_fitting_options.set_slot_for_simultaneous_fit_by_changed(slot)

    def set_slot_for_simultaneous_fit_by_specifier_changed(self, slot) -> None:
        """Connect the slot for the fit specifier combo box being changed."""
        self.general_fitting_options.set_slot_for_simultaneous_fit_by_specifier_changed(slot)

    def update_dataset_name_combo_box(self, dataset_names: list) -> None:
        """Update the data in the parameter display combo box."""
        self.general_fitting_options.update_dataset_name_combo_box(dataset_names)

    def update_global_fit_status(self, fit_statuses: list, index: int) -> None:
        """Updates the global fit status label."""
        if self.simultaneous_fitting_mode and index is not None:
            indexed_fit = fit_statuses[index]
            self.fit_controls.update_global_fit_status_label([indexed_fit == "success"] if indexed_fit else [])
        else:
            super().update_global_fit_status(fit_statuses, index)

    def update_fit_function(self, fit_function: IFunction, global_parameters: list = []) -> None:
        """Updates the parameters of a fit function shown in the view."""
        self.fit_function_options.update_function_browser_parameters(self.simultaneous_fitting_mode, fit_function,
                                                                     global_parameters)

    @property
    def current_dataset_name(self) -> str:
        """Returns the selected dataset name."""
        return self.general_fitting_options.current_dataset_name

    @current_dataset_name.setter
    def current_dataset_name(self, dataset_name: str) -> None:
        """Sets the currently selected dataset name."""
        self.general_fitting_options.current_dataset_name = dataset_name

    def number_of_datasets(self) -> int:
        """Returns the number of dataset names loaded into the widget."""
        return self.general_fitting_options.number_of_datasets()

    @property
    def simultaneous_fit_by(self) -> str:
        """Returns what you are simultaneously fitting by (Run or Group/Pair)."""
        return self.general_fitting_options.simultaneous_fit_by

    @simultaneous_fit_by.setter
    def simultaneous_fit_by(self, fit_by_text: str) -> None:
        """Sets what you are simultaneously fitting by (Run or Group/Pair)."""
        self.general_fitting_options.simultaneous_fit_by = fit_by_text

    @property
    def simultaneous_fit_by_specifier(self) -> str:
        """Returns the run, group or pair name."""
        return self.general_fitting_options.simultaneous_fit_by_specifier

    @property
    def current_dataset_index(self) -> str:
        """Returns the index of the currently displayed dataset."""
        return self.general_fitting_options.current_dataset_index

    def switch_to_simultaneous(self) -> None:
        """Switches the view to simultaneous fit mode."""
        super().switch_to_simultaneous()
        self.general_fitting_options.switch_to_simultaneous()

    def switch_to_single(self) -> None:
        """Switches the view to single fit mode."""
        super().switch_to_single()
        self.general_fitting_options.switch_to_single()

    def hide_simultaneous_fit_options(self) -> None:
        """Hides the simultaneous fit options."""
        self.general_fitting_options.hide_simultaneous_fit_options()

    def disable_simultaneous_fit_options(self) -> None:
        """Disables the simultaneous fit options."""
        self.general_fitting_options.disable_simultaneous_fit_options()

    def enable_simultaneous_fit_options(self) -> None:
        """Enables the simultaneous fit options."""
        self.general_fitting_options.enable_simultaneous_fit_options()

    @property
    def simultaneous_fitting_mode(self) -> bool:
        """Returns true if in simultaneous mode."""
        return self.general_fitting_options.simultaneous_fitting_mode

    @simultaneous_fitting_mode.setter
    def simultaneous_fitting_mode(self, simultaneous: bool) -> None:
        """Sets whether or not you are in simultaneous mode."""
        self.general_fitting_options.simultaneous_fitting_mode = simultaneous

    def setup_fit_by_specifier(self, specifiers: list) -> None:
        """Setup the fit by specifier combo box."""
        self.general_fitting_options.setup_fit_by_specifier(specifiers)
