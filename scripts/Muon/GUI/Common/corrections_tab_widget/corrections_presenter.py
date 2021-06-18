# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.corrections_tab_widget.corrections_model import CorrectionsModel
from Muon.GUI.Common.corrections_tab_widget.corrections_view import CorrectionsView
from Muon.GUI.Common.utilities.load_utils import get_table_workspace_names_from_ADS, load_dead_time_from_filename


class CorrectionsPresenter:
    """
    The CorrectionsPresenter has a CorrectionsView and CorrectionsModel.
    """

    def __init__(self, view: CorrectionsView, model: CorrectionsModel):
        """Initialize the CorrectionsPresenter. Sets up the slots and event observers."""
        self.view = view
        self.model = model

        self.view.set_slot_for_dead_time_from_selector_changed(self.handle_dead_time_from_selector_changed)
        self.view.set_slot_for_dead_time_workspace_selector_changed(self.handle_dead_time_workspace_selector_changed)
        self.view.set_slot_for_dead_time_file_browse_clicked(self.handle_dead_time_browse_clicked)

    def handle_dead_time_from_selector_changed(self) -> None:
        """Handles when the location where the dead time should be retrieved from changes."""
        if self.view.is_dead_time_from_data_file_selected():
            self._handle_dead_time_from_data_file_selected()
            self.view.set_dead_time_data_info_visible(True)
            self.view.set_dead_time_workspace_selector_visible(False)
            self.view.set_dead_time_other_file_visible(False)
        elif self.view.is_dead_time_from_workspace_selected():
            self._handle_dead_time_from_workspace_selected()
            self.view.set_dead_time_data_info_visible(True)
            self.view.set_dead_time_workspace_selector_visible(True)
            self.view.set_dead_time_other_file_visible(False)
        elif self.view.is_dead_time_from_other_file_selected():
            self.view.set_dead_time_data_info_visible(False)
            self.view.set_dead_time_workspace_selector_visible(False)
            self.view.set_dead_time_other_file_visible(True)
        else:
            self._handle_dead_time_from_none_selected()
            self.view.set_dead_time_data_info_visible(False)
            self.view.set_dead_time_workspace_selector_visible(False)
            self.view.set_dead_time_other_file_visible(False)

    def _handle_dead_time_from_data_file_selected(self) -> None:
        """Handles when the dead time from data file is initially selected."""
        self.model.set_dead_time_source_to_from_file()
        self._set_dead_time_info_text_using_average()

    def _handle_dead_time_from_workspace_selected(self) -> None:
        """Handles when the dead time from workspace is initially selected."""
        self.view.populate_dead_time_workspace_selector(get_table_workspace_names_from_ADS())
        self.model.set_dead_time_source_to_from_ADS(self.view.selected_dead_time_workspace())
        self._set_dead_time_info_text_using_average()

    def _handle_dead_time_from_none_selected(self) -> None:
        """Handles when the dead time is none is initially selected."""
        self.model.set_dead_time_source_to_none()
        self._set_dead_time_info_text_using_average()

    def handle_dead_time_workspace_selector_changed(self):
        """The user changes the selected Table Workspace to use as dead time."""
        table_name = self.view.selected_dead_time_workspace()
        if table_name == "None" or table_name == "":
            self._handle_dead_time_from_none_selected()
        else:
            error = self.model.validate_selected_dead_time_workspace(table_name)
            if error == "":
                self.model.set_dead_time_source_to_from_ADS(table_name)
                self._set_dead_time_info_text_using_average()
            else:
                self._handle_selected_table_is_invalid()
                self.view.warning_popup(error)

    def _handle_selected_table_is_invalid(self) -> None:
        """Handles when the selected dead time table workspace is invalid."""
        self.model.set_dead_time_source_to_none()
        self.view.set_dead_time_from_data_file_selected()
        self._set_dead_time_info_text_using_average()

    def handle_dead_time_browse_clicked(self):
        """User selects the option to Browse for a nexus file to load dead times from."""
        filename = self.view.show_file_browser_and_return_selection(["nxs"], [""], multiple_files=False)[0]
        if filename != "":
            name = load_dead_time_from_filename(filename)
            if name != "":
                self.view.switch_to_using_a_dead_time_table_workspace(name)
            else:
                self.view.warning_popup("File does not appear to contain dead time data.")

    def _set_dead_time_info_text_using_average(self) -> None:
        """Sets the dead time average and range text to zero."""
        self.view.set_dead_time_average_and_range(self.model.dead_times_range(), self.model.dead_times_average())
