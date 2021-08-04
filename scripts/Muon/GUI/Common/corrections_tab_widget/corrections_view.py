# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObserver
from mantidqt.utils.qt import load_ui

from Muon.GUI.Common.corrections_tab_widget.background_corrections_view import BackgroundCorrectionsView
from Muon.GUI.Common.corrections_tab_widget.dead_time_corrections_view import DeadTimeCorrectionsView
from Muon.GUI.Common.data_selectors.cyclic_data_selector_view import CyclicDataSelectorView
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
        self.parent = parent

        self.run_selector = CyclicDataSelectorView(self)
        self.run_selector.set_data_combo_box_label("Runs :")
        self.run_selector.set_data_combo_box_label_width(50)
        self.corrections_layout.addWidget(self.run_selector, 1)

        self.dead_time_corrections_view = DeadTimeCorrectionsView(self)
        self.corrections_layout.addWidget(self.dead_time_corrections_view, 1)

        self.background_corrections_view = BackgroundCorrectionsView(self)
        self.corrections_layout.addWidget(self.background_corrections_view, 10)

        self.disable_tab_observer = GenericObserver(self.disable_view)
        self.enable_tab_observer = GenericObserver(self.enable_view)

        self.disable_view()

    @property
    def dead_time_view(self) -> DeadTimeCorrectionsView:
        """Returns the dead time corrections view."""
        return self.dead_time_corrections_view

    @property
    def background_view(self) -> BackgroundCorrectionsView:
        """Returns the background corrections view."""
        return self.background_corrections_view

    def set_tab_warning(self, message: str) -> None:
        """Sets a warning message as the tooltip of the corrections tab."""
        if self.parent is not None:
            self.parent.set_tab_warning("Corrections", message)

    def set_slot_for_run_selector_changed(self, slot) -> None:
        """Connect the slot for the Run Selector combobox"""
        self.run_selector.set_slot_for_dataset_changed(slot)

    def update_run_selector_combo_box(self, runs: list) -> None:
        """Update the data in the run selector combo box."""
        self.run_selector.update_dataset_name_combo_box(runs)

    def current_run_string(self) -> int:
        """Returns the currently displayed run number string."""
        return self.run_selector.current_dataset_name

    def warning_popup(self, message: str) -> None:
        """Displays a warning message."""
        warning(message, parent=self)

    def disable_view(self) -> None:
        """Disable all widgets in this corrections view."""
        self.setEnabled(False)

    def enable_view(self) -> None:
        """Enable all widgets in this corrections view."""
        self.setEnabled(self.run_selector.number_of_datasets() != 0)
