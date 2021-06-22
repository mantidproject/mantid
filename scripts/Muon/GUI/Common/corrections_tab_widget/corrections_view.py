# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui
from Muon.GUI.Common.corrections_tab_widget.dead_time_corrections_view import DeadTimeCorrectionsView
from Muon.GUI.Common.fitting_widgets.basic_fitting.workspace_selector_view import WorkspaceSelectorView
from Muon.GUI.Common.message_box import warning

from qtpy.QtWidgets import QWidget

ui_form, widget = load_ui(__file__, "corrections_view.ui")


class CorrectionsView(widget, ui_form):
    """
    The CorrectionsView contains widgets allowing a Dead Time correction and Background correction.
    """

    def __init__(self, parent: QWidget = None):
        """Initializes the CorrectionsView."""
        super(CorrectionsView, self).__init__(parent)
        self.setupUi(self)

        self.run_selector = WorkspaceSelectorView(self)
        self.run_selector.set_workspace_combo_box_label("Run:")
        self.run_selector.workspace_combo_box_label.setMinimumWidth(50)
        self.run_selector.workspace_combo_box_label.setMaximumWidth(50)
        self.run_selector_layout.addWidget(self.run_selector)

        self.dead_time_corrections_view = DeadTimeCorrectionsView(self)
        self.dead_time_layout.addWidget(self.dead_time_corrections_view)

    def set_slot_for_run_selector_changed(self, slot) -> None:
        """Connect the slot for the Run Selector combobox"""
        self.run_selector.set_slot_for_dataset_changed(slot)

    def set_slot_for_dead_time_from_selector_changed(self, slot) -> None:
        """Connect the slot for the Dead Time Selector Combobox."""
        self.dead_time_corrections_view.set_slot_for_dead_time_from_selector_changed(slot)

    def set_slot_for_dead_time_workspace_selector_changed(self, slot) -> None:
        """Sets the slot for when the selected dead time workspace is changed."""
        self.dead_time_corrections_view.set_slot_for_dead_time_workspace_selector_changed(slot)

    def set_slot_for_dead_time_file_browse_clicked(self, slot) -> None:
        """Sets the slot for the dead time file browse button being clicked."""
        self.dead_time_corrections_view.set_slot_for_dead_time_file_browse_clicked(slot)

    def update_run_selector_combo_box(self, runs: list) -> None:
        """Update the data in the run selector combo box."""
        self.run_selector.update_dataset_name_combo_box(runs)

    def current_run_index(self) -> int:
        """Returns the index of the currently displayed run number."""
        return self.run_selector.current_dataset_index

    def set_dead_time_data_info_visible(self, visible: bool) -> None:
        """Sets the Dead Time Info label as being visible or hidden."""
        self.dead_time_corrections_view.set_dead_time_data_info_visible(visible)

    def set_dead_time_workspace_selector_visible(self, visible: bool) -> None:
        """Sets the Dead Time File Loaded widgets as being visible or hidden."""
        self.dead_time_corrections_view.set_dead_time_workspace_selector_visible(visible)

    def set_dead_time_other_file_visible(self, visible: bool) -> None:
        """Sets the Dead Time Other File widgets as being visible or hidden."""
        self.dead_time_corrections_view.set_dead_time_other_file_visible(visible)

    def is_dead_time_from_data_file_selected(self) -> bool:
        """Returns true if the dead time should be retrieved from a data file."""
        return self.dead_time_corrections_view.is_dead_time_from_data_file_selected()

    def is_dead_time_from_workspace_selected(self) -> bool:
        """Returns true if the dead time should be retrieved from a workspace."""
        return self.dead_time_corrections_view.is_dead_time_from_workspace_selected()

    def is_dead_time_from_other_file_selected(self) -> bool:
        """Returns true if the dead time should be retrieved from some other file."""
        return self.dead_time_corrections_view.is_dead_time_from_other_file_selected()

    def set_dead_time_from_data_file_selected(self) -> None:
        """Sets the dead time selection mode to be from a data file."""
        self.dead_time_corrections_view.set_dead_time_from_data_file_selected()

    def set_dead_time_from_workspace_selected(self):
        """Sets the dead time selection mode to be from a workspace."""
        self.dead_time_corrections_view.set_dead_time_from_workspace_selected()

    def selected_dead_time_workspace(self) -> str:
        """Returns the currently selected dead time table workspace in the workspace selector."""
        return self.dead_time_corrections_view.selected_dead_time_workspace()

    def set_selected_dead_time_workspace(self, table_name) -> str:
        """Returns the currently selected dead time table workspace in the workspace selector."""
        found = self.dead_time_corrections_view.set_selected_dead_time_workspace(table_name)
        if not found:
            self.warning_popup("Dead time table cannot be loaded.")

    def set_dead_time_average_and_range(self, limits: tuple, average: float) -> None:
        """Sets the average dead time and its range in the info label."""
        self.dead_time_corrections_view.set_dead_time_average_and_range(limits, average)

    def set_dead_time_info_text(self, text: str) -> None:
        """Sets the text in the dead time info label."""
        self.dead_time_corrections_view.set_dead_time_info_text(text)

    def populate_dead_time_workspace_selector(self, table_names: list) -> None:
        """Populates the dead time workspace selector with all the names of Table workspaces found in the ADS."""
        self.dead_time_corrections_view.populate_dead_time_workspace_selector(table_names)

    def show_file_browser_and_return_selection(self, extensions: list, search_directories: list,
                                               multiple_files: bool = False) -> list:
        """Opens the file browser and returns the selected file name."""
        return self.dead_time_corrections_view.show_file_browser_and_return_selection(extensions, search_directories,
                                                                                      multiple_files)

    def switch_to_using_a_dead_time_table_workspace(self, table_name: str) -> None:
        """Switch the view to the 'from table workspace' option and provide the table name."""
        self.set_dead_time_from_workspace_selected()
        self.set_selected_dead_time_workspace(table_name)

    def warning_popup(self, message: str) -> None:
        """Displays a warning message."""
        warning(message, parent=self)
