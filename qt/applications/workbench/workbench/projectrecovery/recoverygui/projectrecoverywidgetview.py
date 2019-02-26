# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from __future__ import (absolute_import, unicode_literals)

from qtpy.QtCore import Signal, Slot, Qt
from qtpy.QtWidgets import QDialog, QHeaderView, QTableWidgetItem

from mantid.kernel import logger
from mantidqt.utils.qt import load_ui


class ProjectRecoveryWidgetView(QDialog):
    abort_project_recovery_script = Signal()

    def __init__(self, presenter, parent=None):
        super(ProjectRecoveryWidgetView, self).__init__(parent=parent)
        self.ui = load_ui(__file__, "ProjectRecoveryWidget.ui", baseinstance=self)
        self.presenter = presenter

        # Make sure the UI doesn't leak memory
        self.ui.setAttribute(Qt.WA_DeleteOnClose, True)

        # Set up the ui
        self.ui.tableWidget.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.ui.tableWidget.verticalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.ui.progressBar.setMinimum(0)
        self._add_data_to_table()

    def reject(self):
        self.presenter.start_mantid_normally()

    def set_progress_bar_maximum(self, new_value):
        self.ui.progressBar.setMaximum(new_value)

    def connect_progress_bar(self):
        self.presenter.project_recovery.loader.multi_file_interpreter.current_editor().sig_progress.connect(
            self.update_progress_bar)

    def emit_abort_script(self):
        self.abort_project_recovery_script.connect(
            self.presenter.project_recovery.loader.multi_file_interpreter.abort_all)
        logger.error("Project Recovery: Cancelling recovery")
        self.abort_project_recovery_script.emit()

    def change_start_mantid_button(self, string):
        self.ui.startmantidButton.setText(string)

    def _add_data_to_table(self):
        row = self.presenter.get_row(0)
        self.ui.tableWidget.setItem(0, 0, QTableWidgetItem(row[0]))
        self.ui.tableWidget.setItem(0, 1, QTableWidgetItem(row[1]))

    ######################################################
    #  Slots
    ######################################################

    @Slot(int)
    def update_progress_bar(self, new_value):
        self.ui.progressBar.setValue(new_value)

    # Camel case methods present due to Slot naming in the relevant .ui file
    @Slot()
    def onClickLastCheckpoint(self):
        self.presenter.recover_last()

    @Slot()
    def onClickOpenLastInScriptWindow(self):
        self.presenter.open_last_in_editor()

    @Slot()
    def onClickStartMantidNormally(self):
        self.presenter.start_mantid_normally()
