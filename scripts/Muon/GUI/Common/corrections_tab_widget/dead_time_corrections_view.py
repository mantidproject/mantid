# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui
from Muon.GUI.Common.utilities.muon_file_utils import filter_for_extensions, show_file_browser_and_return_selection

from qtpy.QtWidgets import QWidget

ui_form, widget = load_ui(__file__, "dead_time_corrections_view.ui")

DEAD_TIME_DATA_FILE = "From data file"
DEAD_TIME_WORKSPACE = "From table workspace"
DEAD_TIME_OTHER_FILE = "From other file"


class DeadTimeCorrectionsView(widget, ui_form):
    """
    The DeadTimeCorrectionsView contains the widgets allowing a Dead Time correction.
    """

    def __init__(self, parent: QWidget = None):
        """Initializes the DeadTimeCorrectionsView."""
        super(DeadTimeCorrectionsView, self).__init__(parent)
        self.setupUi(self)

        self.set_dead_time_workspace_selector_visible(False)
        self.set_dead_time_other_file_visible(False)

    def set_slot_for_dead_time_from_selector_changed(self, slot) -> None:
        """Connect the slot for the Dead Time Selector Combobox."""
        self.dead_time_from_selector.currentIndexChanged.connect(slot)

    def set_slot_for_dead_time_workspace_selector_changed(self, slot) -> None:
        """Sets the slot for when the selected dead time workspace is changed."""
        self.dead_time_workspace_selector.currentIndexChanged.connect(slot)

    def set_slot_for_dead_time_file_browse_clicked(self, slot) -> None:
        """Sets the slot for the dead time file browse button being clicked."""
        self.dead_time_browse_button.clicked.connect(slot)

    def set_dead_time_workspace_selector_visible(self, visible: bool) -> None:
        """Sets the Dead Time File Loaded widgets as being visible or hidden."""
        self.dead_time_workspace_label.setVisible(visible)
        self.dead_time_workspace_selector.setVisible(visible)

    def set_dead_time_other_file_visible(self, visible: bool) -> None:
        """Sets the Dead Time Other File widgets as being visible or hidden."""
        self.dead_time_other_file_label.setVisible(visible)
        self.dead_time_browse_button.setVisible(visible)

    def is_dead_time_from_data_file_selected(self) -> bool:
        """Returns true if the dead time should be retrieved from a data file."""
        return self.dead_time_from_selector.currentText() == DEAD_TIME_DATA_FILE

    def is_dead_time_from_workspace_selected(self) -> bool:
        """Returns true if the dead time should be retrieved from a workspace."""
        return self.dead_time_from_selector.currentText() == DEAD_TIME_WORKSPACE

    def is_dead_time_from_other_file_selected(self) -> bool:
        """Returns true if the dead time should be retrieved from some other file."""
        return self.dead_time_from_selector.currentText() == DEAD_TIME_OTHER_FILE

    def set_dead_time_from_data_file_selected(self) -> None:
        """Sets the dead time selection mode to be from a data file."""
        index = self.dead_time_from_selector.findText(DEAD_TIME_DATA_FILE)
        if index != -1:
            self.dead_time_from_selector.setCurrentIndex(index)

    def set_dead_time_from_workspace_selected(self) -> None:
        """Sets the dead time selection mode to be from a workspace."""
        index = self.dead_time_from_selector.findText(DEAD_TIME_WORKSPACE)
        if index != -1:
            self.dead_time_from_selector.setCurrentIndex(index)

    def selected_dead_time_workspace(self) -> str:
        """Returns the currently selected dead time table workspace in the workspace selector."""
        return str(self.dead_time_workspace_selector.currentText())

    def set_selected_dead_time_workspace(self, table_name: str) -> None:
        """Sets the currently selected dead time table workspace in the workspace selector."""
        self.dead_time_workspace_selector.blockSignals(True)
        index = self.dead_time_workspace_selector.findText(table_name)
        if index != -1:
            self.dead_time_workspace_selector.setCurrentIndex(index)
        self.dead_time_workspace_selector.blockSignals(False)

    def set_dead_time_average_and_range(self, run_string: str, limits: tuple, average: float) -> None:
        """Sets the average dead time and its range in the info label."""
        self.dead_time_stats_label.setText(f"Dead Time stats for run {run_string}:")
        self.dead_time_info_label.setText(f"{limits[0]:.3f} to {limits[1]:.3f} (av. {average:.3f})")

    def set_dead_time_info_text(self, text: str) -> None:
        """Sets the text in the dead time info label."""
        self.dead_time_stats_label.setText("")
        self.dead_time_info_label.setText(text)

    def populate_dead_time_workspace_selector(self, table_names: list) -> None:
        """Populates the dead time workspace selector with all the names of Table workspaces found in the ADS."""
        old_name = self.dead_time_workspace_selector.currentText()

        self.dead_time_workspace_selector.blockSignals(True)

        self.dead_time_workspace_selector.clear()
        self.dead_time_workspace_selector.addItems(["None"] + table_names)

        new_index = self.dead_time_workspace_selector.findText(old_name)
        new_index = new_index if new_index != -1 else 0
        self.dead_time_workspace_selector.setCurrentIndex(new_index)

        self.dead_time_workspace_selector.blockSignals(False)

    def show_file_browser_and_return_selection(self, extensions: list, search_directories: list,
                                               multiple_files: bool = False) -> list:
        """Opens the file browser and returns the selected file name."""
        return show_file_browser_and_return_selection(self, filter_for_extensions(extensions), search_directories,
                                                      multiple_files)

    def switch_to_using_a_dead_time_table_workspace(self, table_name: str) -> None:
        """Switch the view to the 'from table workspace' option and provide the table name."""
        self.set_selected_dead_time_workspace(table_name)
        self.set_dead_time_from_workspace_selected()
