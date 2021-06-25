# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserver, GenericObserverWithArgPassing

from Muon.GUI.Common.corrections_tab_widget.corrections_model import CorrectionsModel
from Muon.GUI.Common.corrections_tab_widget.corrections_view import CorrectionsView
from Muon.GUI.Common.utilities.load_utils import get_table_workspace_names_from_ADS, load_dead_time_from_filename

from qtpy.QtCore import QMetaObject, QObject, Slot


class CorrectionsPresenter(QObject):
    """
    The CorrectionsPresenter has a CorrectionsView and CorrectionsModel.
    """

    def __init__(self, view: CorrectionsView, model: CorrectionsModel):
        """Initialize the CorrectionsPresenter. Sets up the slots and event observers."""
        super(CorrectionsPresenter, self).__init__()
        self.view = view
        self.model = model

        self.initialize_model_options()

        self.view.set_slot_for_run_selector_changed(self.handle_run_selector_changed)
        self.view.set_slot_for_dead_time_from_selector_changed(self.handle_dead_time_from_selector_changed)
        self.view.set_slot_for_dead_time_workspace_selector_changed(self.handle_dead_time_workspace_selector_changed)
        self.view.set_slot_for_dead_time_file_browse_clicked(self.handle_dead_time_browse_clicked)

        self.update_view_from_model_observer = GenericObserverWithArgPassing(
            self.handle_ads_clear_or_remove_workspace_event)
        self.instrument_changed_observer = GenericObserver(self.handle_instrument_changed)
        self.load_observer = GenericObserver(self.handle_runs_loaded)
        self.corrections_complete_observer = GenericObserver(self.handle_corrections_complete)

        self.enable_editing_notifier = GenericObservable()
        self.disable_editing_notifier = GenericObservable()
        self.perform_corrections_notifier = GenericObservable()

    def initialize_model_options(self) -> None:
        """Initialise the model with the default fitting options."""
        self.model.set_dead_time_source_to_from_file()

    def handle_ads_clear_or_remove_workspace_event(self, _: str = None) -> None:
        """Handle when there is a clear or remove workspace event in the ADS."""
        if self.model.is_dead_time_source_from_data_file():
            self.view.set_dead_time_from_data_file_selected()
        elif self.model.is_dead_time_source_from_workspace():
            self.view.set_dead_time_from_workspace_selected()

        self.handle_runs_loaded()

    def handle_instrument_changed(self) -> None:
        """User changes the selected instrument."""
        self.set_dead_time_source_to_from_file()
        self.view.set_dead_time_from_data_file_selected()

    def handle_runs_loaded(self) -> None:
        """Handles when new run numbers are loaded. QMetaObject is required so its executed on the GUI thread."""
        QMetaObject.invokeMethod(self, "_handle_runs_loaded")

    @Slot()
    def _handle_runs_loaded(self) -> None:
        """Handles when new run numbers are loaded from the GUI thread."""
        self.view.update_run_selector_combo_box(self.model.run_number_strings())
        self.model.set_current_run_string(self.view.current_run_string())

        if self.model.number_of_run_strings == 0:
            self.view.disable_view()
        else:
            self.view.enable_view()

    def handle_run_selector_changed(self) -> None:
        """Handles when the run selector is changed."""
        self.model.set_current_run_string(self.view.current_run_string())
        self.update_dead_time_info_text_in_view()

    def handle_dead_time_from_selector_changed(self) -> None:
        """Handles when the location where the dead time should be retrieved from changes."""
        if self.view.is_dead_time_from_data_file_selected():
            self._handle_dead_time_from_data_file_selected()
            self.view.set_dead_time_workspace_selector_visible(False)
            self.view.set_dead_time_other_file_visible(False)
        elif self.view.is_dead_time_from_workspace_selected():
            self._handle_dead_time_from_workspace_selected()
            self.view.set_dead_time_workspace_selector_visible(True)
            self.view.set_dead_time_other_file_visible(False)
        elif self.view.is_dead_time_from_other_file_selected():
            self._handle_dead_time_from_none_selected()
            self.view.set_dead_time_workspace_selector_visible(False)
            self.view.set_dead_time_other_file_visible(True)
        else:
            self._handle_dead_time_from_none_selected()
            self.view.set_dead_time_workspace_selector_visible(False)
            self.view.set_dead_time_other_file_visible(False)

    def _handle_dead_time_from_data_file_selected(self) -> None:
        """Handles when the dead time from data file is initially selected."""
        self.set_dead_time_source_to_from_file()

    def _handle_dead_time_from_workspace_selected(self) -> None:
        """Handles when the dead time from workspace is initially selected."""
        self.view.populate_dead_time_workspace_selector(get_table_workspace_names_from_ADS())
        self.set_dead_time_source_to_from_ads()

    def _handle_dead_time_from_none_selected(self) -> None:
        """Handles when the dead time is none is initially selected."""
        self.set_dead_time_source_to_none()

    def handle_dead_time_workspace_selector_changed(self) -> None:
        """The user changes the selected Table Workspace to use as dead time."""
        table_name = self.view.selected_dead_time_workspace()
        if table_name == "None" or table_name == "":
            self._handle_dead_time_from_none_selected()
        else:
            error = self.model.validate_selected_dead_time_workspace(table_name)
            if error == "":
                self.set_dead_time_source_to_from_ads()
            else:
                self.view.set_selected_dead_time_workspace("None")
                self._handle_selected_table_is_invalid()
                self.view.warning_popup(error)

    def _handle_selected_table_is_invalid(self) -> None:
        """Handles when the selected dead time table workspace is invalid."""
        # Triggers handle_dead_time_from_selector_changed
        self.view.set_dead_time_from_data_file_selected()

    def handle_dead_time_browse_clicked(self) -> None:
        """User selects the option to Browse for a nexus file to load dead times from."""
        filename = self.view.show_file_browser_and_return_selection(["nxs"], [""], multiple_files=False)[0]
        if filename != "":
            name = self._load_file_containing_dead_time(filename)
            if name is not None:
                self.view.populate_dead_time_workspace_selector(get_table_workspace_names_from_ADS())
                error = self.model.validate_selected_dead_time_workspace(name)
                if error == "":
                    self.view.switch_to_using_a_dead_time_table_workspace(name)
                else:
                    self.view.warning_popup(error)

    def handle_corrections_complete(self) -> None:
        """When the corrections have been calculated, update the displayed dead time averages."""
        self.update_dead_time_info_text_in_view()

    def update_dead_time_info_text_in_view(self) -> None:
        """Update the dead time info label in the view."""
        if self.model.is_dead_time_source_from_data_file() or self.model.is_dead_time_source_from_workspace():
            self.view.set_dead_time_average_and_range(self.model.dead_times_range(), self.model.dead_times_average())
        else:
            self.view.set_dead_time_info_text("No dead time correction")

    def set_dead_time_source_to_from_file(self) -> None:
        """Sets the dead time source to be from the data file and notifies the GUI to recalculate the corrections."""
        self.model.set_dead_time_source_to_from_file()
        self._notify_perform_dead_time_corrections()

    def set_dead_time_source_to_from_ads(self) -> None:
        """Sets the dead time source to be the ADS and notifies the GUI to recalculate the corrections."""
        self.model.set_dead_time_source_to_from_ads(self.view.selected_dead_time_workspace())
        self._notify_perform_dead_time_corrections()

    def set_dead_time_source_to_none(self) -> None:
        """Sets the dead time source to be none and notifies the GUI to recalculate the corrections."""
        self.model.set_dead_time_source_to_none()
        self._notify_perform_dead_time_corrections()

    def _load_file_containing_dead_time(self, filename: str) -> str:
        """Attempts to load a Nexus cycle file containing a dead time table workspace."""
        try:
            name = load_dead_time_from_filename(filename)
        except Exception:
            self.view.warning_popup("The file provided has an unexpected format. A Nexus cycle file must be provided.")
            return None

        if name == "":
            self.view.warning_popup("The file provided does not contain dead time data.")
            return None
        return name

    def _notify_perform_dead_time_corrections(self) -> None:
        """A notification event to trigger the calculation of the dead time corrections."""
        self.disable_editing_notifier.notify_subscribers()
        self.perform_corrections_notifier.notify_subscribers()
        self.enable_editing_notifier.notify_subscribers()
