# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

from qtpy.QtCore import Qt
from qtpy.QtGui import QDropEvent
from qtpy.QtWidgets import QTableWidget, QAbstractItemView, QTableWidgetItem, QWidget, QHBoxLayout, \
    QApplication

from mantidqt.utils.qt import load_ui
import functools
from Muon.GUI.Common.utilities import table_utils

ui_list_selector, _ = load_ui(__file__, "list_selector.ui")


class ListSelectorView(QtWidgets.QWidget, ui_list_selector):
    def __init__(self, parent_widget=None):
        super(QtWidgets.QWidget, self).__init__(parent=parent_widget)
        self.setupUi(self)

        self.item_table_widget.setColumnWidth(0, 350)

        self._item_selection_changed_action = lambda a, b: 0

    def addItems(self, item_list):
        """
        Adds all the items in item list to the table
        :param itemList: A list of tuples (item_name, check_state, enabled)
        """
        for index, row in enumerate(item_list):
            insertion_index = self.item_table_widget.rowCount()
            self.item_table_widget.setRowCount(insertion_index + 1)
            table_utils.setRowName(self.item_table_widget, insertion_index, row[0])
            check_box = table_utils.addCheckBoxWidgetToTable(self.item_table_widget, row[1] ,insertion_index, 1)
            item_selection_changed_action_with_row_encoded = functools.partial(self._item_selection_changed_action, row[0])
            check_box.stateChanged.connect(item_selection_changed_action_with_row_encoded)
            self.set_row_enabled(insertion_index, row[2])

    def clearItems(self):
        for row in range(self.item_table_widget.rowCount()):
            self.item_table_widget.takeItem(row, 0)
            self.item_table_widget.removeCellWidget(row, 1)
        self.item_table_widget.setRowCount(0)

    def set_row_enabled(self, row, enabled):
        for col in range(self.item_table_widget.columnCount()):
            item = self.item_table_widget.cellWidget(row, col)
            if item:
                item.setEnabled(enabled)

    def set_filter_line_edit_changed_action(self, action):
        self.filter_line_edit.textChanged.connect(action)

    def set_item_selection_changed_action(self, action):
        self._item_selection_changed_action = action

    def set_filter_type_combo_changed_action(self, action):
        self.filter_type_combo_box.currentIndexChanged.connect(action)


class TableWidgetDragRows(QTableWidget):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.setDragEnabled(True)
        self.setAcceptDrops(True)
        self.viewport().setAcceptDrops(True)
        self.setDragDropOverwriteMode(False)
        self.setDropIndicatorShown(True)

        self.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.setDragDropMode(QAbstractItemView.InternalMove)

    def dropEvent(self, event: QDropEvent):
        if not event.isAccepted() and event.source() == self:
            drop_row = self.drop_on(event)

            rows = sorted(set(item.row() for item in self.selectedItems()))
            rows_to_move = [[QTableWidgetItem(self.item(row_index, column_index)) for column_index in range(self.columnCount())]
                            for row_index in rows]
            for row_index in reversed(rows):
                self.removeRow(row_index)
                if row_index < drop_row:
                    drop_row -= 1

            for row_index, data in enumerate(rows_to_move):
                row_index += drop_row
                self.insertRow(row_index)
                for column_index, column_data in enumerate(data):
                    self.setItem(row_index, column_index, column_data)
            event.accept()
            for row_index in range(len(rows_to_move)):
                self.item(drop_row + row_index, 0).setSelected(True)
                self.item(drop_row + row_index, 1).setSelected(True)
        super().dropEvent(event)

    def drop_on(self, event):
        index = self.indexAt(event.pos())
        if not index.isValid():
            return self.rowCount()

        return index.row() + 1 if self.is_below(event.pos(), index) else index.row()

    def is_below(self, pos, index):
        rect = self.visualRect(index)
        margin = 2
        if pos.y() - rect.top() < margin:
            return False
        elif rect.bottom() - pos.y() < margin:
            return True
        # noinspection PyTypeChecker
        return rect.contains(pos, True) and not (int(self.model().flags(index)) & Qt.ItemIsDropEnabled) and pos.y() >= rect.center().y()
