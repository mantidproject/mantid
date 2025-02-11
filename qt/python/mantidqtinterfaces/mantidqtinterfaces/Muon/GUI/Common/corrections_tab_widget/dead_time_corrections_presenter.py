# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.corrections_tab_widget.dead_time_corrections_model import DeadTimeCorrectionsModel
from mantidqtinterfaces.Muon.GUI.Common.corrections_tab_widget.dead_time_corrections_view import DeadTimeCorrectionsView
from mantidqtinterfaces.Muon.GUI.Common.utilities.load_utils import get_table_workspace_names_from_ADS, load_dead_time_from_filename


class DeadTimeCorrectionsPresenter:
    """
    The DeadTimeCorrectionsPresenter has a DeadTimeCorrectionsView and DeadTimeCorrectionsModel.
    """

    def __init__(self, view: DeadTimeCorrectionsView, model: DeadTimeCorrectionsModel, corrections_presenter):
        """Initialize the DeadTimeCorrectionsPresenter. Sets up the slots and event observers."""
        self.view = view
        self.model = model
        self._corrections_presenter = corrections_presenter

        self.view.set_slot_for_dead_time_from_selector_changed(self.handle_dead_time_from_selector_changed)
        self.view.set_slot_for_dead_time_workspace_selector_changed(self.handle_dead_time_workspace_selector_changed)
        self.view.set_slot_for_dead_time_file_browse_clicked(self.handle_dead_time_browse_clicked)

    def initialize_model_options(self) -> None:
        """Initialise the model with the default fitting options."""
        self.model.set_dead_time_source_to_from_file()

    def handle_ads_clear_or_remove_workspace_event(self, _: str = None) -> None:
        """Handle when there is a clear or remove workspace event in the ADS."""
        if self.model.is_dead_time_source_from_data_file():
            self.view.set_dead_time_from_data_file_selected()
        elif self.model.is_dead_time_source_from_workspace():
            self.view.set_dead_time_from_workspace_selected()

    def handle_instrument_changed(self) -> None:
        """User changes the selected instrument."""
        self.model.set_dead_time_source_to_from_file()
        self.view.set_dead_time_from_data_file_selected()

    def handle_run_selector_changed(self) -> None:
        """Handles when the run selector is changed."""
        if self.model.is_dead_time_source_from_data_file():
            self.model.set_dead_time_source_to_from_file()
        self.update_dead_time_info_text_in_view()

    def handle_dead_time_from_selector_changed(self) -> None:
        """Handles when the location where the dead time should be retrieved from changes."""
        if self.view.is_dead_time_from_data_file_selected():
            self._handle_dead_time_from_data_file_selected()
            self._set_dead_time_widgets_visible(False, False)
        elif self.view.is_dead_time_from_workspace_selected():
            self._handle_dead_time_from_workspace_selected()
            self._set_dead_time_widgets_visible(True, False)
        elif self.view.is_dead_time_from_other_file_selected():
            self._handle_dead_time_from_none_selected()
            self._set_dead_time_widgets_visible(False, True)
        else:
            self._handle_dead_time_from_none_selected()
            self._set_dead_time_widgets_visible(False, False)

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
                self._corrections_presenter.warning_popup(error)

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
                    self._corrections_presenter.warning_popup(error)

    def handle_pre_process_and_counts_calculated(self) -> None:
        """Handles when MuonPreProcess and counts workspaces have been calculated."""
        self.update_dead_time_info_text_in_view()

    def update_dead_time_info_text_in_view(self) -> None:
        """Update the dead time info label in the view."""
        if self.model.is_dead_time_source_from_data_file() or self.model.is_dead_time_source_from_workspace():
            self.view.set_dead_time_average_and_range(
                self._corrections_presenter.current_run_string(), self.model.dead_times_range(), self.model.dead_times_average()
            )
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

    def _set_dead_time_widgets_visible(self, workspace_mode_visible: bool, other_file_mode_visible: bool) -> None:
        """Sets which dead time widgets are visible."""
        self.view.set_dead_time_workspace_selector_visible(workspace_mode_visible)
        self.view.set_dead_time_other_file_visible(other_file_mode_visible)

    def _load_file_containing_dead_time(self, filename: str) -> str:
        """Attempts to load a Nexus cycle file containing a dead time table workspace."""
        try:
            name = load_dead_time_from_filename(filename)
        except Exception:
            self._corrections_presenter.warning_popup(
                "The file provided has an unexpected format. The file should be of the same instrument and cycle as the raw data."
            )
            return None

        if name == "":
            self._corrections_presenter.warning_popup("The file provided does not contain dead time data.")
            return None
        return name

    def _notify_perform_dead_time_corrections(self) -> None:
        """A notification event to trigger the calculation of the dead time corrections."""
        self._corrections_presenter.disable_editing_notifier.notify_subscribers()
        self._corrections_presenter.perform_corrections_notifier.notify_subscribers()
        self._corrections_presenter.enable_editing_notifier.notify_subscribers()
