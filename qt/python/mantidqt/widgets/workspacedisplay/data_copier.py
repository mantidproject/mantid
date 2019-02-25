# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, print_function)

from qtpy import QtGui
from qtpy.QtWidgets import QMessageBox

from mantidqt.widgets.workspacedisplay.user_notifier import UserNotifier

"""
This module contains the common copying functionality between
the MatrixWorkspaceDisplay and the TableWorkspaceDisplay.
"""


class DataCopier(UserNotifier):
    def __init__(self, status_bar):
        super(DataCopier, self).__init__(status_bar)

    def copy_spectrum_values(self, table, ws_read):
        """
        Copies the values selected by the user to the system's clipboard

        :param table: Table from which the selection will be read
        :param ws_read: The workspace read function, that is used to access the data directly
        """
        selection_model = table.selectionModel()
        if not selection_model.hasSelection():
            self.notify_no_selection_to_copy()
            return
        selected_rows = selection_model.selectedRows()  # type: list
        row_data = []

        for index in selected_rows:
            row = index.row()
            data = "\t".join(map(str, ws_read(row)))

            row_data.append(data)

        self.copy_to_clipboard("\n".join(row_data))
        self.notify_successful_copy()

    def copy_bin_values(self, table, ws_read, num_rows):
        """
        Copies the values selected by the user to the system's clipboard

        :param table: Table from which the selection will be read
        :param ws_read: The workspace read function, that is used to access the data directly
        :param num_rows: The number of rows in the column
        """
        selection_model = table.selectionModel()
        if not selection_model.hasSelection():
            self.notify_no_selection_to_copy()
            return

        self.notify_working()

        selected_columns = selection_model.selectedColumns()  # type: list

        # Qt gives back a QModelIndex, we need to extract the column from it
        column_data = []
        for index in selected_columns:
            column = index.column()
            data = [str(ws_read(row)[column]) for row in range(num_rows)]
            column_data.append(data)

        all_string_rows = []
        for i in range(num_rows):
            # Appends ONE value from each COLUMN, this is because the final string is being built vertically
            # the noqa disables a 'data' variable redefined warning
            all_string_rows.append("\t".join([data[i] for data in column_data]))  # noqa: F812

        # Finally all rows are joined together with a new line at the end of each row
        final_string = "\n".join(all_string_rows)
        self.copy_to_clipboard(final_string)
        self.notify_successful_copy()

    def copy_cells(self, table):
        """
        :type table: QTableView
        :param table: The table from which the data will be copied.
        :return:
        """
        selectionModel = table.selectionModel()
        if not selectionModel.hasSelection():
            self.notify_no_selection_to_copy()
            return

        selection = selectionModel.selection()
        selectionRange = selection.first()

        top = selectionRange.top()
        bottom = selectionRange.bottom()
        left = selectionRange.left()
        right = selectionRange.right()

        data = []
        index = selectionModel.currentIndex()
        for i in range(top, bottom + 1):
            for j in range(left, right):
                data.append(str(index.sibling(i, j).data()))
                data.append("\t")
            data.append(str(index.sibling(i, right).data()))
            data.append("\n")

        # strip the string to remove the trailing new line
        self.copy_to_clipboard("".join(data).strip())
        self.notify_successful_copy()

    def copy_to_clipboard(self, data):
        """
        Uses the QGuiApplication to copy to the system clipboard.

        :type data: str
        :param data: The data that will be copied to the clipboard
        :return:
        """
        cb = QtGui.QGuiApplication.clipboard()
        cb.setText(data, mode=cb.Clipboard)

    def ask_confirmation(self, message, title="Mantid Workbench"):
        reply = QMessageBox.question(self, title, message, QMessageBox.Yes, QMessageBox.No)
        return True if reply == QMessageBox.Yes else False
