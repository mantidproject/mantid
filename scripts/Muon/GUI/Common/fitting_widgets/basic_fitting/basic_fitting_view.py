# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import IFunction
from mantidqt.utils.qt import load_ui

from Muon.GUI.Common.fitting_widgets.basic_fitting.fit_controls_view import FitControlsView
from Muon.GUI.Common.fitting_widgets.basic_fitting.fit_function_options_view import FitFunctionOptionsView
from Muon.GUI.Common.message_box import warning

from qtpy.QtWidgets import QWidget

ui_fitting_layout, _ = load_ui(__file__, "fitting_layout.ui")


class BasicFittingView(QWidget, ui_fitting_layout):
    """
    The BasicFittingView has a FitControlsView and a FitFunctionOptionsView. It can be used for Single Fitting.
    """

    def __init__(self, parent: QWidget = None, is_frequency_domain: bool = False):
        """Initialize the BasicFittingView and create the FitControlsView and a FitFunctionOptionsView."""
        super(BasicFittingView, self).__init__(parent)
        self.setupUi(self)

        self.fit_controls = FitControlsView(self)
        self.fit_function_options = FitFunctionOptionsView(self, is_frequency_domain)

        self.fit_controls_layout.addWidget(self.fit_controls)
        self.fit_function_options_layout.addWidget(self.fit_function_options)

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

    def set_datasets_in_function_browser(self, dataset_names: list) -> None:
        """Sets the datasets stored in the FunctionBrowser."""
        self.fit_function_options.set_datasets_in_function_browser(dataset_names)

    def set_current_dataset_index(self, dataset_index: int) -> None:
        """Sets the index of the current dataset."""
        if dataset_index is not None:
            self.fit_function_options.set_current_dataset_index(dataset_index)

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
    def fit_object(self) -> IFunction:
        """Returns the global fitting function."""
        return self.fit_function_options.fit_object

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

    def enable_undo_fit(self, enable: bool) -> None:
        """Sets whether or not undo fit is enabled."""
        self.fit_controls.enable_undo_fit(enable)

    @property
    def function_name(self) -> str:
        """Returns the function name being used."""
        return self.fit_function_options.function_name

    @function_name.setter
    def function_name(self, function_name: str) -> None:
        """Sets the function name being used."""
        self.fit_function_options.function_name = function_name

    def number_of_datasets(self) -> int:
        """Returns the number of dataset names loaded into the widget."""
        return self.fit_function_options.number_of_datasets()

    def warning_popup(self, message: str) -> None:
        """Displays a warning message."""
        warning(message, parent=self)

    @property
    def global_parameters(self) -> list:
        """Returns a list of global parameters."""
        return self.fit_function_options.global_parameters

    def switch_to_simultaneous(self) -> None:
        """Switches the view to simultaneous fit mode."""
        self.fit_function_options.switch_to_simultaneous()

    def switch_to_single(self) -> None:
        """Switches the view to single fit mode."""
        self.fit_function_options.switch_to_single()
