# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Dict, Any

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QMessageBox

from mantidqt.io import open_a_file_dialog
from mantidqt.utils.qt import load_ui

form, base = load_ui(__file__, "main.ui")


class SettingsView(base, form):
    ASK_BEFORE_CLOSE_TITLE = "Confirm exit"
    ASK_BEFORE_CLOSE_MESSAGE = "Are you sure you want to exit without applying the settings?\n\n"
    CHANGES_NEED_RESTART_TITLE = "Some changes require restart"
    CHANGES_NEED_RESTART_MESSAGE = "The following changes will be applied when the workbench is restarted:\n\n"

    def __init__(self, parent, presenter):
        super(SettingsView, self).__init__(parent)
        self.setupUi(self)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        self.presenter = presenter

        self.sections.currentRowChanged.connect(self.presenter.action_section_changed)

    def closeEvent(self, event):
        if not self.presenter.view_closing():
            event.ignore()
        else:
            self.deleteLater()
            super(SettingsView, self).closeEvent(event)

    def ask_before_close(self, unsaved_changes: Dict[str, Any]):
        change_strings = [f"{property_string}: {value}" for property_string, value in unsaved_changes.items()]
        reply = QMessageBox.question(
            self,
            self.ASK_BEFORE_CLOSE_TITLE,
            self.ASK_BEFORE_CLOSE_MESSAGE + "  • " + "\n  • ".join(change_strings),
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.No,
        )
        return True if reply == QMessageBox.Yes else False

    def get_properties_filename(self, accept_mode, file_mode):
        return open_a_file_dialog(
            parent=self,
            default_suffix=".properties",
            file_filter="PROPERTIES file (*.properties)",
            accept_mode=accept_mode,
            file_mode=file_mode,
        )

    def notify_changes_need_restart(self, list_of_changes_that_need_restart):
        if list_of_changes_that_need_restart:
            QMessageBox.information(
                self,
                self.CHANGES_NEED_RESTART_TITLE,
                self.CHANGES_NEED_RESTART_MESSAGE + "  • " + "\n  • ".join(list_of_changes_that_need_restart),
                QMessageBox.Ok,
            )
