# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from __future__ import absolute_import, unicode_literals

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QMessageBox

from mantidqt.utils.qt import load_ui

form, base = load_ui(__file__, "main.ui")


class SettingsView(base, form):
    def __init__(self, parent, presenter):
        super(SettingsView, self).__init__(parent)
        self.setupUi(self)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        self.presenter = presenter

        self.sections.currentRowChanged.connect(self.presenter.action_section_changed)

    def closeEvent(self, event):
        self.presenter.refresh_workspaces()
        self.deleteLater()
        super(SettingsView, self).closeEvent(event)

    def ask_before_close(self):
        reply = QMessageBox.question(self, self.presenter.ASK_BEFORE_CLOSE_TITLE,
                                     self.presenter.ASK_BEFORE_CLOSE_MESSAGE, QMessageBox.Yes, QMessageBox.No)
        return True if reply == QMessageBox.Yes else False
