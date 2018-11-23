# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)

from functools import partial

from qtpy import QtGui
from qtpy.QtCore import Qt
from qtpy.QtGui import QKeySequence
from qtpy.QtWidgets import (QAction, QHeaderView, QMessageBox, QTableView, QTableWidget)

import mantidqt.icons


class TableWorkspaceDisplayView(QTableWidget):
    def __init__(self, presenter, parent=None, name=''):
        super(TableWorkspaceDisplayView, self).__init__(parent)

        self.presenter = presenter
        self.COPY_ICON = mantidqt.icons.get_icon("fa.files-o")
        self.DELETE_ROW = mantidqt.icons.get_icon("fa.minus-square-o")
        self.STATISTICS_ON_ROW = mantidqt.icons.get_icon('fa.fighter-jet')

        # change the default color of the rows - makes them light blue
        # monitors and masked rows are colored in the table's custom model
        # palette = self.palette()
        # palette.setColor(QtGui.QPalette.Base, QtGui.QColor(128, 255, 255))
        # self.setPalette(palette)

        self.setWindowTitle("{} - Mantid".format(name))
        self.setWindowFlags(Qt.Window)

        self.resize(600, 400)
        self.show()

    def doubleClickedHeader(self):
        print("Double clicked WOO")

    def keyPressEvent(self, event):
        if event.matches(QKeySequence.Copy):
            self.presenter.action_keypress_copy(self)

    def set_context_menu_actions(self, table):
        """
        Sets up the context menu actions for the table
        :type table: QTableView
        :param table: The table whose context menu actions will be set up.
        :param ws_read_function: The read function used to efficiently retrieve data directly from the workspace
        """
        copy_action = QAction(self.COPY_ICON, "Copy", table)
        # sets the first (table) parameter of the copy action callback
        # so that each context menu can copy the data from the correct table
        decorated_copy_action_with_correct_table = partial(self.presenter.action_copy_cells, table)
        copy_action.triggered.connect(decorated_copy_action_with_correct_table)

        table.setContextMenuPolicy(Qt.ActionsContextMenu)
        table.addAction(copy_action)

        horizontalHeader = table.horizontalHeader()
        horizontalHeader.setContextMenuPolicy(Qt.ActionsContextMenu)
        horizontalHeader.setSectionResizeMode(QHeaderView.Fixed)

        copy_bin_values = QAction(self.COPY_ICON, "Copy", horizontalHeader)
        copy_bin_values.triggered.connect(self.presenter.action_copy_bin_values)

        horizontalHeader.addAction(copy_bin_values)

        verticalHeader = table.verticalHeader()
        verticalHeader.setContextMenuPolicy(Qt.ActionsContextMenu)
        verticalHeader.setSectionResizeMode(QHeaderView.Fixed)

        copy_spectrum_values = QAction(self.COPY_ICON, "Copy", verticalHeader)
        copy_spectrum_values.triggered.connect(self.presenter.action_copy_spectrum_values)

        delete_row = QAction(self.DELETE_ROW, "Delete Row", verticalHeader)
        delete_row.triggered.connect(self.presenter.action_delete_row)

        statistics_on_rows = QAction(self.STATISTICS_ON_ROW, "Statistics on Rows", verticalHeader)
        statistics_on_rows.triggered.connect(self.presenter.action_statistics_on_rows)

        separator1 = QAction(verticalHeader)
        separator1.setSeparator(True)
        separator2 = QAction(verticalHeader)
        separator2.setSeparator(True)

        verticalHeader.addAction(copy_spectrum_values)
        verticalHeader.addAction(separator1)
        verticalHeader.addAction(delete_row)
        verticalHeader.addAction(separator2)
        verticalHeader.addAction(statistics_on_rows)

    @staticmethod
    def copy_to_clipboard(data):
        """
        Uses the QGuiApplication to copy to the system clipboard.

        :type data: str
        :param data: The data that will be copied to the clipboard
        :return:
        """
        cb = QtGui.QGuiApplication.clipboard()
        cb.setText(data, mode=cb.Clipboard)

    def ask_confirmation(self, message, title="Mantid Workbench"):
        """
        :param message:
        :return:
        """
        reply = QMessageBox.question(self, title, message, QMessageBox.Yes, QMessageBox.No)
        return True if reply == QMessageBox.Yes else False
