from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import pyqtSignal as Signal

from Muon.GUI.Common import message_box
from Muon.GUI.Common.utilities import table_utils

pair_columns = {0: 'pair_name', 1: 'group_1', 2: 'group_2', 3: 'alpha', 4: 'guess_alpha'}


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

        # Table entry validation
        self._validate_pair_name_entry = lambda text: True
        self._validate_alpha = lambda text: True

        self._on_table_data_changed = lambda: 0
        self._on_guess_alpha_clicked = lambda row: 0

        # The active groups that can be selected from the group combo box
        self._group_selections = []

        # whether the table is updating and therefore we shouldn't respond to signals
        self._updating = False

        # the right-click context menu
        self.menu = None
        self._disabled = False
        self.add_pair_action = None
        self.remove_pair_action = None

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
        self.add_pair_button.setObjectName("addGroupButton")
        self.add_pair_button.setToolTip("Add a pair to the end of the table")
        self.add_pair_button.setText("+")

        self.remove_pair_button.setSizePolicy(size_policy)
        self.remove_pair_button.setObjectName("removeGroupButton")
        self.remove_pair_button.setToolTip("Remove selected/last pair(s) from the table")
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
        self.pairing_table.setColumnCount(5)
        self.pairing_table.setHorizontalHeaderLabels(["Pair Name", "Group 1", " Group 2", "Alpha", "Guess Alpha"])
        header = self.pairing_table.horizontalHeader()
        header.setResizeMode(0, QtGui.QHeaderView.Stretch)
        header.setResizeMode(1, QtGui.QHeaderView.Stretch)
        header.setResizeMode(2, QtGui.QHeaderView.Stretch)
        header.setResizeMode(3, QtGui.QHeaderView.Stretch)
        header.setResizeMode(4, QtGui.QHeaderView.ResizeToContents)
        vertical_headers = self.pairing_table.verticalHeader()
        vertical_headers.setMovable(False)
        vertical_headers.setResizeMode(QtGui.QHeaderView.ResizeToContents)
        vertical_headers.setVisible(True)

        self.pairing_table.horizontalHeaderItem(0).setToolTip("The name of the pair :"
                                                              "\n    - The name must be unique across all groups/pairs"
                                                              "\n    - The name can only use digits, characters and _")
        self.pairing_table.horizontalHeaderItem(1).setToolTip("Group 1 of the pair, selected from the grouping table")
        self.pairing_table.horizontalHeaderItem(2).setToolTip("Group 2 of the pair, selected from the grouping table")
        self.pairing_table.horizontalHeaderItem(3).setToolTip("The value of Alpha for the pair asymmetry:"
                                                              "\n   - The number must be >= 0.0")
        self.pairing_table.horizontalHeaderItem(4).setToolTip("Replace the current value of Alpha with one estimated"
                                                              " from the data.")

    def num_rows(self):
        return self.pairing_table.rowCount()

    def num_cols(self):
        return self.pairing_table.columnCount()

    def update_group_selections(self, group_name_list):
        self._group_selections = group_name_list

    def get_index_of_text(self, selector, text):
        for i in range(selector.count()):
            if str(selector.itemText(i)) == text:
                return i
        return 0

    def clear(self):
        # Go backwards to preserve indices
        for row in reversed(range(self.num_rows())):
            self.pairing_table.removeRow(row)

    def notify_data_changed(self):
        if not self._updating:
            self.dataChanged.emit()

    def add_entry_to_table(self, row_entries):
        assert len(row_entries) == self.pairing_table.columnCount() - 1

        row_position = self.pairing_table.rowCount()
        self.pairing_table.insertRow(row_position)
        for i, entry in enumerate(row_entries):
            item = QtGui.QTableWidgetItem(entry)
            if pair_columns[i] == 'pair_name':
                pair_name_widget = table_utils.ValidatedTableItem(self._validate_pair_name_entry)
                pair_name_widget.setText(entry)
                self.pairing_table.setItem(row_position, i, pair_name_widget)
                item.setFlags(QtCore.Qt.ItemIsEnabled)
                item.setFlags(QtCore.Qt.ItemIsSelectable)
            if pair_columns[i] == 'group_1':
                group1_selector_widget = self._group_selection_cell_widget()
                # ensure changing the selection sends an update signal
                group1_selector_widget.currentIndexChanged.connect(lambda: self.on_cell_changed(row_position, 1))
                index = self.get_index_of_text(group1_selector_widget, entry)
                group1_selector_widget.setCurrentIndex(index)
                self.pairing_table.setCellWidget(row_position, i, group1_selector_widget)
            if pair_columns[i] == 'group_2':
                group2_selector_widget = self._group_selection_cell_widget()
                # ensure changing the selection sends an update signal
                group2_selector_widget.currentIndexChanged.connect(lambda: self.on_cell_changed(row_position, 2))
                index = self.get_index_of_text(group2_selector_widget, entry)
                group2_selector_widget.setCurrentIndex(index)
                self.pairing_table.setCellWidget(row_position, i, group2_selector_widget)
            if pair_columns[i] == 'alpha':
                alpha_widget = table_utils.ValidatedTableItem(self._validate_alpha)
                alpha_widget.setText(entry)
                self.pairing_table.setItem(row_position, i, alpha_widget)
            self.pairing_table.setItem(row_position, i, item)
        # guess alpha button
        guess_alpha_widget = self._guess_alpha_button()
        guess_alpha_widget.clicked.connect(lambda: self.guess_alpha_clicked_from_row(row_position))
        self.pairing_table.setCellWidget(row_position, 4, guess_alpha_widget)

    def _group_selection_cell_widget(self):
        # The widget for the group selection columns
        selector = QtGui.QComboBox(self)
        selector.setToolTip("Select a group from the grouping table")
        selector.addItems(self._group_selections)
        return selector

    def _guess_alpha_button(self):
        # The widget for the guess alpha column
        guess_alpha = QtGui.QPushButton(self)
        guess_alpha.setToolTip("Estimate the alpha value for this pair")
        guess_alpha.setText("Guess")
        return guess_alpha

    def guess_alpha_clicked_from_row(self, row):
        self._on_guess_alpha_clicked(row)

    def get_table_contents(self):
        if self._updating:
            return []
        ret = [[None for _ in range(self.num_cols())] for _ in range(self.num_rows())]
        for row in range(self.num_rows()):
            for col in range(self.num_cols()):
                if col == 1 or col == 2:
                    # columns with widgets
                    ret[row][col] = str(self.pairing_table.cellWidget(row, col).currentText())
                elif col == 4:
                    ret[row][col] = "Guess"
                else:
                    # columns without widgets
                    ret[row][col] = str(self.pairing_table.item(row, col).text())
        return ret

    def get_table_item_text(self, row, col):
        return self.pairing_table.item(row, col).text()

    # ------------------------------------------------------------------------------------------------------------------
    # Signal / Slot connections
    # ------------------------------------------------------------------------------------------------------------------

    def on_user_changes_pair_name(self, slot):
        self._validate_pair_name_entry = slot

    def on_user_changes_alpha(self, slot):
        self._validate_alpha = slot

    def on_guess_alpha_clicked(self, slot):
        self._on_guess_alpha_clicked = slot

    def on_add_pair_button_clicked(self, slot):
        self.add_pair_button.clicked.connect(slot)

    def on_remove_pair_button_clicked(self, slot):
        self.remove_pair_button.clicked.connect(slot)

    def on_table_data_changed(self, slot):
        self._on_table_data_changed = slot

    def on_item_changed(self):
        """Not yet implemented."""
        if not self._updating:
            pass

    def on_cell_changed(self, _row, _col):
        if not self._updating:
            self._on_table_data_changed(_row, _col)

    # ------------------------------------------------------------------------------------------------------------------
    # Context Menu
    # ------------------------------------------------------------------------------------------------------------------

    def contextMenuEvent(self, _event):
        """Overridden method for dealing with the right-click context menu"""
        self.menu = QtGui.QMenu(self)

        self.add_pair_action = self._context_menu_add_pair_action(self.add_pair_button.clicked.emit)
        self.remove_pair_action = self._context_menu_remove_pair_action(self.remove_pair_button.clicked.emit)

        if self._disabled:
            self.add_pair_action.setEnabled(False)
            self.remove_pair_action.setEnabled(False)
        # set-up the menu
        self.menu.addAction(self.add_pair_action)
        self.menu.addAction(self.remove_pair_action)
        self.menu.popup(QtGui.QCursor.pos())

    def _context_menu_add_pair_action(self, slot):
        add_pair_action = QtGui.QAction('Add Pair', self)
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

    # ------------------------------------------------------------------------------------------------------------------
    # Adding / Removing pairs
    # ------------------------------------------------------------------------------------------------------------------

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

    def enter_pair_name(self):
        new_pair_name, ok = QtGui.QInputDialog.getText(self, 'Pair Name', 'Enter name of new pair:')
        if ok:
            return new_pair_name

    # ------------------------------------------------------------------------------------------------------------------
    # Enabling / Disabling the table
    # ------------------------------------------------------------------------------------------------------------------

    def enable_updates(self):
        """Allow update signals to be sent."""
        self._updating = False

    def disable_updates(self):
        """Prevent update signals being sent."""
        self._updating = True

    def enable_editing(self):
        self.disable_updates()
        self._disabled = False
        self._enable_all_buttons()
        self._enable_all_table_items()
        self.enable_updates()

    def disable_editing(self):
        self.disable_updates()
        self._disabled = True
        self._disable_all_buttons()
        self._disable_all_table_items()
        self.enable_updates()

    def _disable_all_table_items(self):
        for row in range(self.num_rows()):
            for col in range(self.num_cols()):
                if col == 1 or col == 2 or col == 4:
                    item = self.pairing_table.cellWidget(row, col)
                    item.setEnabled(False)
                else:
                    item = self.pairing_table.item(row, col)
                    item.setFlags(QtCore.Qt.ItemIsSelectable)

    def _enable_all_table_items(self):
        for row in range(self.num_rows()):
            for col in range(self.num_cols()):
                if col == 1 or col == 2 or col == 4:
                    item = self.pairing_table.cellWidget(row, col)
                    item.setEnabled(True)
                elif col == 0:
                    item = self.pairing_table.item(row, col)
                    item.setFlags(QtCore.Qt.ItemIsSelectable)
                else:
                    item = self.pairing_table.item(row, col)
                    item.setFlags(QtCore.Qt.ItemIsSelectable |
                                  QtCore.Qt.ItemIsEditable |
                                  QtCore.Qt.ItemIsEnabled)

    def _enable_all_buttons(self):
        self.add_pair_button.setEnabled(True)
        self.remove_pair_button.setEnabled(True)

    def _disable_all_buttons(self):
        self.add_pair_button.setEnabled(False)
        self.remove_pair_button.setEnabled(False)
