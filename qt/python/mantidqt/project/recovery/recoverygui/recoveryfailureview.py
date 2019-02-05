# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from qtpy.QtWidgets import QDialog, QHeaderView, QTableWidgetItem
from qtpy.QtCore import Signal, Slot, QObject
from mantidqt.utils.qt import load_ui


class RecoveryFailureView(QDialog):
    def __init__(self, presenter):
        super(RecoveryFailureView, self).__init__()
        self.ui = load_ui(__file__, "RecoveryFailure.ui", baseinstance=self)
        self.ui.tableWidget.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.ui.tableWidget.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.presenter = presenter

        # Make sure that the presenter has all the data we need
        self.presenter.fill_all_rows()

        # Set the table data
        self._add_data_table()

    def _add_data_to_table(self):
        # This table's size was generated for 5 which is the default but will take the
        number_of_rows = self.presenter.get_number_of_checkpoints()
        for ii in range(0, number_of_rows):
            row = self.presenter.get_row(ii)
            for jj in range(0, row.size()):
                self.ui.tableWidget.setItem(ii, jj, QTableWidgetItem(row[jj]))

    def reject(self):
        self.presenter.start_mantid_normally()

    def set_progress_bar_maximum(self, new_value):
        self.ui.progressBar.setValue(new_value)

    def connect_progress_bar(self):
        QObject.connect(self.presenter.project_recovery.multi_file_interpreter.current_editor(),
                        self.presenter.project_recovery.multi_file_interpreter.current_editor().sig_progress(int),
                        self, self.update_progress_bar(int))

    def emit_abort_script(self):
        #todo: make a connection between this, with signal abort, and mainwindow's scripthandler to abort
        self.abort_project_recovery_script.emit()

    def change_start_mantid_button(self, string):
        self.ui.pushButton_3.setText(string)

    ######################################################
    #  Signals
    ######################################################

    abort_project_recovery_script = Signal()

    ######################################################
    #  Slots
    ######################################################

    @Slot(int)
    def update_progress_bar(self, new_value):
        self.ui.progressBar.setValue(new_value)

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
                self.presenter.open_selected_in_editor(text)

    @Slot()
    def onClickStartMantidNormally(self):
        self.presenter.start_mantid_normally()