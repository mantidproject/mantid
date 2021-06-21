# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import IFunction
from mantidqt.utils.observer_pattern import GenericObserver
from mantidqt.utils.qt import load_ui

from Muon.GUI.Common.fitting_widgets.basic_fitting.fit_controls_view import FitControlsView
from Muon.GUI.Common.fitting_widgets.basic_fitting.fit_function_options_view import FitFunctionOptionsView
from Muon.GUI.Common.fitting_widgets.basic_fitting.workspace_selector_view import WorkspaceSelectorView
from Muon.GUI.Common.message_box import warning

from qtpy.QtWidgets import QWidget

ui_form, base_widget = load_ui(__file__, "fitting_layout.ui")


class BasicFittingView(ui_form, base_widget):
    """
    The BasicFittingView has a FitControlsView and a FitFunctionOptionsView. It can be used for Single Fitting.
    """

    def __init__(self, parent: QWidget = None):
        """Initialize the BasicFittingView and create the FitControlsView and a FitFunctionOptionsView."""
        super(BasicFittingView, self).__init__(parent)
        self.setupUi(self)

        self.fit_controls = FitControlsView(self)
        self.workspace_selector = WorkspaceSelectorView(self)
        self.fit_function_options = FitFunctionOptionsView(self)

        self.fit_controls_layout.addWidget(self.fit_controls)
        self.workspace_selector_layout.addWidget(self.workspace_selector)
        self.fit_function_options_layout.addWidget(self.fit_function_options)

        self.disable_tab_observer = GenericObserver(self.disable_view)
        self.enable_tab_observer = GenericObserver(self.enable_view)

        self.disable_view()

    def set_slot_for_fit_generator_clicked(self, slot) -> None:
        """Connect the slot for the Fit Generator button."""
        self.fit_controls.set_slot_for_fit_generator_clicked(slot)

    def set_slot_for_fit_button_clicked(self, slot) -> None:
        """Connect the slot for the Fit button."""
        self.fit_controls.set_slot_for_fit_button_clicked(slot)

    def set_slot_for_undo_fit_clicked(self, slot) -> None:
        """Connect the slot for the Undo Fit button."""
        self.fit_controls.set_slot_for_undo_fit_clicked(slot)

    def set_slot_for_plot_guess_changed(self, slot) -> None:
        """Connect the slot for the Plot Guess checkbox."""
        self.fit_controls.set_slot_for_plot_guess_changed(slot)

    def set_slot_for_dataset_changed(self, slot) -> None:
        """Connect the slot for the display workspace combo box being changed."""
        self.workspace_selector.set_slot_for_dataset_changed(slot)

    def set_slot_for_fit_name_changed(self, slot) -> None:
        """Connect the slot for the fit name being changed by the user."""
        self.fit_function_options.set_slot_for_fit_name_changed(slot)

    def set_slot_for_function_structure_changed(self, slot) -> None:
        """Connect the slot for the function structure changing."""
        self.fit_function_options.set_slot_for_function_structure_changed(slot)

    def set_slot_for_function_parameter_changed(self, slot) -> None:
        """Connect the slot for a function parameter changing."""
        self.fit_function_options.set_slot_for_function_parameter_changed(slot)

    def set_slot_for_start_x_updated(self, slot) -> None:
        """Connect the slot for the start x option."""
        self.fit_function_options.set_slot_for_start_x_updated(slot)

    def set_slot_for_end_x_updated(self, slot) -> None:
        """Connect the slot for the end x option."""
        self.fit_function_options.set_slot_for_end_x_updated(slot)

    def set_slot_for_minimizer_changed(self, slot) -> None:
        """Connect the slot for changing the Minimizer."""
        self.fit_function_options.set_slot_for_minimizer_changed(slot)

    def set_slot_for_evaluation_type_changed(self, slot) -> None:
        """Connect the slot for changing the Evaluation type."""
        self.fit_function_options.set_slot_for_evaluation_type_changed(slot)

    def set_slot_for_use_raw_changed(self, slot) -> None:
        """Connect the slot for the Use raw option."""
        self.fit_function_options.set_slot_for_use_raw_changed(slot)

    def set_workspace_combo_box_label(self, text: str) -> None:
        """Sets the label text next to the workspace selector combobox."""
        self.workspace_selector.set_workspace_combo_box_label(text)

    def set_datasets_in_function_browser(self, dataset_names: list) -> None:
        """Sets the datasets stored in the FunctionBrowser."""
        self.fit_function_options.set_datasets_in_function_browser(dataset_names)

    def set_current_dataset_index(self, dataset_index: int) -> None:
        """Sets the index of the current dataset."""
        if dataset_index is not None:
            self.fit_function_options.set_current_dataset_index(dataset_index)

    def set_number_of_undos(self, number_of_undos: int) -> None:
        """Sets the allowed number of 'Undo Fit' events."""
        self.fit_controls.set_number_of_undos(number_of_undos)

    def update_dataset_name_combo_box(self, dataset_names: list) -> None:
        """Update the data in the parameter display combo box."""
        self.workspace_selector.update_dataset_name_combo_box(dataset_names)

    def update_local_fit_status_and_chi_squared(self, fit_status: str, chi_squared: float) -> None:
        """Updates the view to show the status and results from a fit."""
        if fit_status is not None:
            self.fit_function_options.update_fit_status_labels(fit_status, chi_squared)
        else:
            self.fit_function_options.clear_fit_status()

    def update_global_fit_status(self, fit_statuses: list, _: int = None) -> None:
        """Updates the global fit status label."""
        self.fit_controls.update_global_fit_status_label([status == "success" for status in fit_statuses if status])

    def update_fit_function(self, fit_function: IFunction) -> None:
        """Updates the parameters of a fit function shown in the view."""
        self.fit_function_options.update_function_browser_parameters(False, fit_function)

    @property
    def current_dataset_name(self) -> str:
        """Returns the selected dataset name."""
        return self.workspace_selector.current_dataset_name

    @current_dataset_name.setter
    def current_dataset_name(self, dataset_name: str) -> None:
        """Sets the currently selected dataset name."""
        self.workspace_selector.current_dataset_name = dataset_name

    def number_of_datasets(self) -> int:
        """Returns the number of dataset names loaded into the widget."""
        return self.workspace_selector.number_of_datasets()

    @property
    def current_dataset_index(self) -> int:
        """Returns the index of the currently displayed dataset."""
        return self.workspace_selector.current_dataset_index

    @property
    def fit_object(self) -> IFunction:
        """Returns the global fitting function."""
        return self.fit_function_options.fit_object

    def current_fit_function(self) -> IFunction:
        """Returns the current fitting function in the view."""
        return self.fit_function_options.current_fit_function()

    @property
    def minimizer(self) -> str:
        """Returns the selected minimizer."""
        return self.fit_function_options.minimizer

    @property
    def start_x(self) -> float:
        """Returns the selected start X."""
        return self.fit_function_options.start_x

    @start_x.setter
    def start_x(self, value: float) -> None:
        """Sets the selected start X."""
        self.fit_function_options.start_x = value

    @property
    def end_x(self) -> float:
        """Returns the selected end X."""
        return self.fit_function_options.end_x

    @end_x.setter
    def end_x(self, value: float) -> None:
        """Sets the selected end X."""
        self.fit_function_options.end_x = value

    @property
    def evaluation_type(self) -> str:
        """Returns the selected evaluation type."""
        return self.fit_function_options.evaluation_type

    @property
    def fit_to_raw(self) -> bool:
        """Returns whether or not fitting to raw data is ticked."""
        return self.fit_function_options.fit_to_raw

    @fit_to_raw.setter
    def fit_to_raw(self, check: bool) -> None:
        """Sets whether or not you are fitting to raw data."""
        self.fit_function_options.fit_to_raw = check

    @property
    def plot_guess(self) -> bool:
        """Returns true if plot guess is ticked."""
        return self.fit_controls.plot_guess

    @plot_guess.setter
    def plot_guess(self, check: bool) -> None:
        """Sets whether or not plot guess is ticked."""
        self.fit_controls.plot_guess = check

    @property
    def function_name(self) -> str:
        """Returns the function name being used."""
        return self.fit_function_options.function_name

    @function_name.setter
    def function_name(self, function_name: str) -> None:
        """Sets the function name being used."""
        self.fit_function_options.function_name = function_name

    def warning_popup(self, message: str) -> None:
        """Displays a warning message."""
        warning(message, parent=self)

    @property
    def global_parameters(self) -> list:
        """Returns a list of global parameters."""
        return self.fit_function_options.global_parameters

    def parameter_value(self, full_parameter: str) -> float:
        """Returns the value of the specified parameter."""
        return self.fit_function_options.parameter_value(full_parameter)

    def switch_to_simultaneous(self) -> None:
        """Switches the view to simultaneous fit mode."""
        self.fit_function_options.switch_to_simultaneous()

    def switch_to_single(self) -> None:
        """Switches the view to single fit mode."""
        self.fit_function_options.switch_to_single()

    def hide_fit_raw_checkbox(self) -> None:
        """Hides the Fit Raw checkbox in the fitting options."""
        self.fit_function_options.hide_fit_raw_checkbox()

    def set_start_and_end_x_labels(self, start_x_label: str, end_x_label: str) -> None:
        """Sets the labels to use for the start and end X labels in the fit options table."""
        self.fit_function_options.set_start_and_end_x_labels(start_x_label, end_x_label)

    def disable_view(self) -> None:
        """Disable all widgets in this fitting widget."""
        self.setEnabled(False)

    def enable_view(self) -> None:
        """Enable all widgets in this fitting widget."""
        self.setEnabled(True)
