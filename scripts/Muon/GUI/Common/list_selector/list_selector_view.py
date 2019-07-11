# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

from qtpy import QtWidgets
import functools
from qtpy.QtCore import Qt
from Muon.GUI.Common.list_selector.TableWidgetDragRows import TableWidgetDragRows
from mantidqt.utils.qt import load_ui
from Muon.GUI.Common.utilities import table_utils

ui_list_selector, _ = load_ui(__file__, "list_selector.ui")
CHECKBOX_COLUMN = 0
WORKSPACE_NAME_COLUMN = 1


class ListSelectorView(QtWidgets.QWidget, ui_list_selector):
    def __init__(self, parent_widget=None):
        super(QtWidgets.QWidget, self).__init__(parent=parent_widget)
        self.setupUi(self)

        self.item_table_widget = TableWidgetDragRows()

        self.item_table_widget.setColumnCount(2)
        self.item_table_widget.setColumnWidth(CHECKBOX_COLUMN, 10)
        self.item_table_widget.verticalHeader().setVisible(False)
        self.item_table_widget.horizontalHeader().setStretchLastSection(True)
        self.item_table_widget.horizontalHeader().setVisible(False)

        self.list_layout.addWidget(self.item_table_widget)
        self.list_layout.setContentsMargins(0, 0, 0, 0)

        self._item_selection_changed_action = lambda name, selection_state: 0
        self.item_table_widget.cellChanged.connect(self.handle_cell_changed_signal)

    def addItems(self, item_list):
        """
        Adds all the items in item list to the table
        :param itemList: A list of tuples (item_name, check_state, enabled)
        """
        self.item_table_widget.blockSignals(True)
        for index, row in enumerate(item_list):
            insertion_index = self.item_table_widget.rowCount()
            self.item_table_widget.setRowCount(insertion_index + 1)
            table_utils.setRowName(self.item_table_widget, insertion_index, row[0], col=WORKSPACE_NAME_COLUMN)
            table_utils.addCheckBoxToTable(self.item_table_widget, row[1], insertion_index, CHECKBOX_COLUMN)
            self.set_row_enabled(insertion_index, row[2])
        self.item_table_widget.blockSignals(False)

    def clearItems(self):
        self.item_table_widget.blockSignals(True)
        self.item_table_widget.clearContents()
        self.item_table_widget.setRowCount(0)
        self.item_table_widget.blockSignals(False)

    def set_row_enabled(self, row, enabled):
        item = self.item_table_widget.item(row, WORKSPACE_NAME_COLUMN)
        if enabled:
            item.setFlags(
                Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsDragEnabled | Qt.ItemIsDropEnabled)
        else:
            item.setFlags(
                Qt.ItemIsSelectable | Qt.ItemIsDragEnabled | Qt.ItemIsDropEnabled)
        item = self.item_table_widget.item(row, CHECKBOX_COLUMN)
        if enabled:
            item.setFlags(
                Qt.ItemIsUserCheckable | Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsDragEnabled | Qt.ItemIsDropEnabled)
        else:
            item.setFlags(
                Qt.ItemIsUserCheckable | Qt.ItemIsSelectable | Qt.ItemIsDragEnabled | Qt.ItemIsDropEnabled)

    def set_filter_line_edit_changed_action(self, action):
        self.filter_line_edit.textChanged.connect(action)

    def set_item_selection_changed_action(self, action):
        self._item_selection_changed_action = action

    def set_filter_type_combo_changed_action(self, action):
        self.filter_type_combo_box.currentIndexChanged.connect(action)

    def set_select_all_checkbox_action(self, action):
        self.select_all_button.clicked.connect(functools.partial(action, True))
        self.unselect_all_button.clicked.connect(functools.partial(action, False))

    def set_show_selected_checkbox_changed(self, action):
        self.show_selected_checkbox.stateChanged.connect(action)

    def set_row_moved_checkbox_action(self, action):
        self.item_table_widget.rowMoved.connect(action)

    def handle_cell_changed_signal(self, row, col):
        if col == CHECKBOX_COLUMN:
            name = self.item_table_widget.item(row, WORKSPACE_NAME_COLUMN).text()
            state = self.item_table_widget.item(row, CHECKBOX_COLUMN).checkState() == Qt.Checked
            self._item_selection_changed_action(name, state)

    def update_number_of_selected_label(self, number_selected, number_selected_displayed):
        self.number_of_selected_display_box.setText('Displaying {} of {} selected'.format(number_selected, number_selected_displayed))

    def disable_filtering_options(self):
        self.filter_line_edit.setEnabled(False)
        self.filter_type_combo_box.setEnabled(False)

    def enable_filtering_options(self):
        self.filter_line_edit.setEnabled(True)
        self.filter_type_combo_box.setEnabled(True)
