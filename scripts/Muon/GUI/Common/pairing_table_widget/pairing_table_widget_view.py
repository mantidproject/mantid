from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import pyqtSignal as Signal

from Muon.GUI.Common import message_box, table_utils


class PairingTableView(QtGui.QWidget):
    dataChanged = Signal()

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super(PairingTableView, self).__init__(parent)

        self.pairing_table = QtGui.QTableWidget(self)
        self.set_up_table()

        self.setup_interface_layout()

        self.pairing_table.itemChanged.connect(self.on_item_changed)
        self.pairing_table.cellChanged.connect(self.on_cell_changed)

        self._validate_pair_name_entry = lambda text: True
        self._validate_alpha = lambda text: True

        self._on_table_data_changed = lambda: 0

        # The active groups that can be selected from the group combo box
        self._group_selections = []

        # whether the table is updating and therefore
        # we shouldn't respond to signals
        self._updating = False

        # Flag for context menus
        self._disabled = False

    def disable_editing(self):
        self.disable_updates()
        self._disabled = True
        self.add_pair_button.setEnabled(False)
        self.remove_pair_button.setEnabled(False)
        for i in range(self.num_rows()):
            for j in range(4):
                try:
                    item = self.pairing_table.item(i, j)
                    item.setFlags(QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled)
                except:
                    item = self.pairing_table.cellWidget(i, j)
                    item.setEnabled(False)
        self.enable_updates()

    def enable_editing(self):
        self.disable_updates()
        self._disabled = False
        self.add_pair_button.setEnabled(True)
        self.remove_pair_button.setEnabled(True)
        for i in range(self.num_rows()):
            for j in range(4):
                try:
                    item = self.pairing_table.item(i, j)
                    item.setFlags(QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEditable | QtCore.Qt.ItemIsEnabled)
                except:
                    item = self.pairing_table.cellWidget(i, j)
                    item.setEnabled(True)
        self.enable_updates()

    def update_group_selections(self, group_name_list):
        self._group_selections = group_name_list

    def on_user_changes_pair_name(self, slot):
        self._validate_pair_name_entry = slot

    def on_user_changes_alpha(self, slot):
        self._validate_alpha = slot

    def setup_interface_layout(self):
        self.setObjectName("PairingTableView")
        self.resize(500, 500)

        self.add_pair_button = QtGui.QToolButton()
        self.remove_pair_button = QtGui.QToolButton()

        size_policy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(self.add_pair_button.sizePolicy().hasHeightForWidth())
        size_policy.setHeightForWidth(self.remove_pair_button.sizePolicy().hasHeightForWidth())

        self.add_pair_button.setSizePolicy(size_policy)
        self.add_pair_button.setMinimumSize(QtCore.QSize(25, 25))
        self.add_pair_button.setObjectName("addGroupButton")
        self.add_pair_button.setText("+")

        self.remove_pair_button.setSizePolicy(size_policy)
        self.remove_pair_button.setMinimumSize(QtCore.QSize(25, 25))
        self.remove_pair_button.setObjectName("removeGroupButton")
        self.remove_pair_button.setText("-")

        self.horizontal_layout = QtGui.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.add_pair_button)
        self.horizontal_layout.addWidget(self.remove_pair_button)
        self.spacer_item = QtGui.QSpacerItem(QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Minimum)
        self.horizontal_layout.addItem(self.spacer_item)
        self.horizontal_layout.setAlignment(QtCore.Qt.AlignLeft)

        self.vertical_layout = QtGui.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addWidget(self.pairing_table)
        self.vertical_layout.addLayout(self.horizontal_layout)

        self.setLayout(self.vertical_layout)

    def set_up_table(self):
        self.pairing_table.setColumnCount(4)
        self.pairing_table.setHorizontalHeaderLabels(["Pair Name", "Group 1", "Group 2", "Alpha"])
        # QtCore.QString("Pair Name;Group 1; Group 2;Alpha").split(";"))
        header = self.pairing_table.horizontalHeader()
        header.setResizeMode(0, QtGui.QHeaderView.Stretch)
        header.setResizeMode(1, QtGui.QHeaderView.Stretch)
        header.setResizeMode(2, QtGui.QHeaderView.Stretch)
        header.setResizeMode(3, QtGui.QHeaderView.ResizeToContents)
        vertical_headers = self.pairing_table.verticalHeader()
        vertical_headers.setMovable(False)
        vertical_headers.setResizeMode(QtGui.QHeaderView.ResizeToContents)
        vertical_headers.setVisible(True)

    def _context_menu_add_pair_action(self, slot):
        add_pair_action = QtGui.QAction('Add Group', self)
        if len(self._get_selected_row_indices()) > 0:
            add_pair_action.setEnabled(False)
        add_pair_action.triggered.connect(slot)
        return add_pair_action

    def _context_menu_remove_pair_action(self, slot):
        if len(self._get_selected_row_indices()) > 1:
            # use plural if >1 item selected
            remove_pair_action = QtGui.QAction('Remove Pairs', self)
        else:
            remove_pair_action = QtGui.QAction('Remove Pair', self)
        if self.num_rows() == 0:
            remove_pair_action.setEnabled(False)
        remove_pair_action.triggered.connect(slot)
        return remove_pair_action

    def contextMenuEvent(self, _event):
        """Overridden method"""
        self.menu = QtGui.QMenu(self)

        add_pair_action = self._context_menu_add_pair_action(self.add_pair_button.clicked.emit)
        remove_pair_action = self._context_menu_remove_pair_action(self.remove_pair_button.clicked.emit)

        if self._disabled:
            add_pair_action.setEnabled(False)
            remove_pair_action.setEnabled(False)

        self.menu.addAction(add_pair_action)
        self.menu.addAction(remove_pair_action)

        self.menu.popup(QtGui.QCursor.pos())

    def _group_selection_cell_widget(self):
        selector = QtGui.QComboBox(self)
        selector.addItems(self._group_selections)
        return selector

    def get_index_of_text(self, selector, text):
        for i in range(selector.count()):
            if str(selector.itemText(i)) == text:
                return i
        return 0

    def add_entry_to_table(self, row_entries):
        assert len(row_entries) == self.pairing_table.columnCount()

        row_position = self.pairing_table.rowCount()
        self.pairing_table.insertRow(row_position)
        for i, entry in enumerate(row_entries):
            item = QtGui.QTableWidgetItem(entry)
            if i == 0:
                pair_name_widget = table_utils.ValidatedTableItem(self._validate_pair_name_entry)
                pair_name_widget.setText(entry)
                self.pairing_table.setItem(row_position, 0, pair_name_widget)
                continue
            if i == 1:
                group1_selector_widget = self._group_selection_cell_widget()
                index = self.get_index_of_text(group1_selector_widget, entry)
                group1_selector_widget.setCurrentIndex(index)
                self.pairing_table.setCellWidget(row_position, 1, group1_selector_widget)
                continue
            if i == 2:
                group2_selector_widget = self._group_selection_cell_widget()
                index = self.get_index_of_text(group2_selector_widget, entry)
                group2_selector_widget.setCurrentIndex(index)
                self.pairing_table.setCellWidget(row_position, 2, group2_selector_widget)
                continue
            if i == 3:
                alpha_widget = table_utils.ValidatedTableItem(self._validate_alpha)
                alpha_widget.setText(entry)
                self.pairing_table.setItem(row_position, 3, alpha_widget)
                continue
                # item.setFlags(QtCore.Qt.ItemIsEnabled)
                # item.setFlags(QtCore.Qt.ItemIsSelectable)
            self.pairing_table.setItem(row_position, i, item)

    def on_add_pair_button_clicked(self, slot):
        self.add_pair_button.clicked.connect(slot)

    def on_remove_pair_button_clicked(self, slot):
        self.remove_pair_button.clicked.connect(slot)

    def on_table_data_changed(self, slot):
        self._on_table_data_changed = slot

    def _get_selected_row_indices(self):
        return list(set(index.row() for index in self.pairing_table.selectedIndexes()))

    def get_selected_pair_names(self):
        indexes = self._get_selected_row_indices()
        return [str(self.pairing_table.item(i, 0).text()) for i in indexes]

    def remove_selected_pairs(self):
        indices = self._get_selected_row_indices()
        for index in reversed(sorted(indices)):
            self.pairing_table.removeRow(index)

    def remove_last_row(self):
        last_row = self.pairing_table.rowCount() - 1
        if last_row >= 0:
            self.pairing_table.removeRow(last_row)

    def num_rows(self):
        return self.pairing_table.rowCount()

    def on_item_changed(self):
        """Not yet implemented."""
        pass

    def on_cell_changed(self, _row, _col):
        if not self._updating:
            self._on_table_data_changed()

    def get_table_contents(self):
        if self._updating:
            return []
        ret = [[None for _ in range(4)] for _ in range(self.num_rows())]
        for i in range(self.num_rows()):
            for j in range(4):
                if j == 1 or j == 2:
                    ret[i][j] = str(self.pairing_table.cellWidget(i, j).currentText())
                else:
                    ret[i][j] = str(self.pairing_table.item(i, j).text())
        return ret

    def clear(self):
        # Go backwards to preserve indices
        for row in reversed(range(self.num_rows())):
            self.pairing_table.removeRow(row)

    def notify_data_changed(self):
        if not self._updating:
            self.dataChanged.emit()

    def disable_updates(self):
        self._updating = True

    def enable_updates(self):
        self._updating = False
