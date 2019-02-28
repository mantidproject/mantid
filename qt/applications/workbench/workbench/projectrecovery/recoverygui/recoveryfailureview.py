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


class RecoveryFailureView(QDialog):
    abort_project_recovery_script = Signal()

    def __init__(self, presenter, parent=None):
        super(RecoveryFailureView, self).__init__(parent=parent)
        self.ui = load_ui(__file__, "RecoveryFailure.ui", baseinstance=self)
        self.ui.tableWidget.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.ui.tableWidget.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.presenter = presenter

        # Make sure the UI doesn't leak memory
        self.ui.setAttribute(Qt.WA_DeleteOnClose, True)

        # Set the table data
        self._add_data_to_table()

    def _add_data_to_table(self):
        # This table's size was generated for 5 which is the default but will take the value given by the presenter
        number_of_rows = self.presenter.get_number_of_checkpoints()
        for ii in range(0, number_of_rows):
            row = self.presenter.get_row(ii)
            for jj in range(0, len(row)):
                self.ui.tableWidget.setItem(ii, jj, QTableWidgetItem(row[jj]))

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
        self.ui.pushButton_3.setText(string)

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
    def onClickSelectedCheckpoint(self):
        selected_rows = self.ui.tableWidget.selectedItems()
        if len(selected_rows) > 0:
            text = selected_rows[0].text()
            if text == "":
                return
            else:
                self.presenter.recover_selected_checkpoint(text)

    @Slot()
    def onClickOpenSelectedInScriptWindow(self):
        selected_rows = self.ui.tableWidget.selectedItems()
        if len(selected_rows) > 0:
            text = selected_rows[0].text()
            if text == "":
                return
            else:
                self.presenter.open_selected_checkpoint_in_editor(text)

    @Slot()
    def onClickStartMantidNormally(self):
        self.presenter.start_mantid_normally()
