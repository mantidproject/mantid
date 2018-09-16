from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import pyqtSignal as Signal

from Muon.GUI.Common import table_utils
from Muon.GUI.Common import message_box


class GroupingTableView(QtGui.QWidget):
    # For use by parent widget
    dataChanged = Signal()
    addPairRequested = Signal(str, str)

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super(GroupingTableView, self).__init__(parent)

        self.grouping_table = QtGui.QTableWidget(self)
        self.set_up_table()

        self.setup_interface_layout()

        self.grouping_table.itemChanged.connect(self.on_item_changed)
        self.grouping_table.cellChanged.connect(self.on_cell_changed)

        self._validate_group_name_entry = lambda text: True
        self._validate_detector_ID_entry = lambda text: True
        self._on_table_data_changed = lambda: 0

        # whether the table is updating and therefore
        # we shouldn't respond to signals
        self._updating = False
        self._disabled = False

    def disable_editing(self):
        self.disable_updates()
        self._disabled = True
        self.add_group_button.setEnabled(False)
        self.remove_group_button.setEnabled(False)
        self._disable_all_table_items()
        self.enable_updates()

    def _disable_all_table_items(self):
        for row in range(self.num_rows()):
            for col in range(3):
                item = self.grouping_table.item(row, col)
                item.setFlags(QtCore.Qt.ItemIsSelectable |
                              QtCore.Qt.ItemIsEnabled)

    def enable_editing(self):
        self.disable_updates()
        self._disabled = False
        self.add_group_button.setEnabled(True)
        self.remove_group_button.setEnabled(True)
        self._enable_all_table_items()
        self.enable_updates()

    def _enable_all_table_items(self):
        for row in range(self.num_rows()):
            for col in range(3):
                item = self.grouping_table.item(row, col)
                if col != 2:
                    item.setFlags(QtCore.Qt.ItemIsSelectable |
                                  QtCore.Qt.ItemIsEditable |
                                  QtCore.Qt.ItemIsEnabled)
                else:
                    item.setFlags(QtCore.Qt.ItemIsSelectable)

    def on_user_changes_group_name(self, slot):
        self._validate_group_name_entry = slot

    def on_user_changes_detector_IDs(self, slot):
        self._validate_detector_ID_entry = slot

    def setup_interface_layout(self):
        self.setObjectName("GroupingTableView")
        self.resize(500, 500)

        self.add_group_button = QtGui.QToolButton()
        self.remove_group_button = QtGui.QToolButton()

        size_policy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(self.add_group_button.sizePolicy().hasHeightForWidth())
        size_policy.setHeightForWidth(self.remove_group_button.sizePolicy().hasHeightForWidth())

        self.add_group_button.setSizePolicy(size_policy)
        self.add_group_button.setMinimumSize(QtCore.QSize(25, 25))
        self.add_group_button.setObjectName("addGroupButton")
        self.add_group_button.setText("+")

        self.remove_group_button.setSizePolicy(size_policy)
        self.remove_group_button.setMinimumSize(QtCore.QSize(25, 25))
        self.remove_group_button.setObjectName("removeGroupButton")
        self.remove_group_button.setText("-")

        self.horizontal_layout = QtGui.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.add_group_button)
        self.horizontal_layout.addWidget(self.remove_group_button)
        self.spacer_item = QtGui.QSpacerItem(QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Minimum)
        self.horizontal_layout.addItem(self.spacer_item)
        self.horizontal_layout.setAlignment(QtCore.Qt.AlignLeft)

        self.vertical_layout = QtGui.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addWidget(self.grouping_table)
        self.vertical_layout.addLayout(self.horizontal_layout)

        self.setLayout(self.vertical_layout)

    def set_up_table(self):
        self.grouping_table.setColumnCount(3)
        self.grouping_table.setHorizontalHeaderLabels(["Group Name", "Detector IDs", "N Detectors"])
            #QtCore.QString("Group Name;Detector IDs;N Detectors").split(";"))
        header = self.grouping_table.horizontalHeader()
        header.setResizeMode(0, QtGui.QHeaderView.Stretch)
        header.setResizeMode(1, QtGui.QHeaderView.Stretch)
        header.setResizeMode(2, QtGui.QHeaderView.ResizeToContents)
        vertical_headers = self.grouping_table.verticalHeader()
        vertical_headers.setMovable(False)
        vertical_headers.setResizeMode(QtGui.QHeaderView.ResizeToContents)
        vertical_headers.setVisible(True)

    def add_pair_requested(self):
        selected_names = self.get_selected_group_names()
        self.addPairRequested.emit(selected_names[0], selected_names[1])

    def _context_menu_add_group_action(self, slot):
        add_group_action = QtGui.QAction('Add Group', self)
        if len(self._get_selected_row_indices()) > 0:
            add_group_action.setEnabled(False)
        add_group_action.triggered.connect(slot)
        return add_group_action

    def _context_menu_remove_group_action(self, slot):
        if len(self._get_selected_row_indices()) > 1:
            # use plural if >1 item selected
            remove_group_action = QtGui.QAction('Remove Groups', self)
        else:
            remove_group_action = QtGui.QAction('Remove Group', self)
        if self.num_rows() == 0:
            remove_group_action.setEnabled(False)
        remove_group_action.triggered.connect(slot)
        return remove_group_action

    def _context_menu_add_pair_action(self, slot):
        add_pair_action = QtGui.QAction('Add Pair', self)
        if len(self._get_selected_row_indices()) != 2:
            add_pair_action.setEnabled(False)
        add_pair_action.triggered.connect(slot)
        return add_pair_action

    def contextMenuEvent(self, _event):
        """Overridden method"""
        self.menu = QtGui.QMenu(self)

        add_group_action = self._context_menu_add_group_action(self.add_group_button.clicked.emit)
        remove_group_action = self._context_menu_remove_group_action(self.remove_group_button.clicked.emit)
        add_pair_action = self._context_menu_add_pair_action(self.add_pair_requested)

        if self._disabled:
            add_group_action.setEnabled(False)
            remove_group_action.setEnabled(False)
            add_pair_action.setEnabled(False)

        self.menu.addAction(add_group_action)
        self.menu.addAction(remove_group_action)
        self.menu.addAction(add_pair_action)

        self.menu.popup(QtGui.QCursor.pos())

    def add_entry_to_table(self, row_entries):
        assert len(row_entries) == self.grouping_table.columnCount()

        row_position = self.grouping_table.rowCount()
        self.grouping_table.insertRow(row_position)
        for i, entry in enumerate(row_entries):
            item = QtGui.QTableWidgetItem(entry)
            if i == 0:
                group_name_widget = table_utils.ValidatedTableItem(self._validate_group_name_entry)
                group_name_widget.setText(entry)
                self.grouping_table.setItem(row_position, 0, group_name_widget)
                continue
            if i == 1:
                detector_widget = table_utils.ValidatedTableItem(self._validate_detector_ID_entry)
                detector_widget.setText(entry)
                self.grouping_table.setItem(row_position, 1, detector_widget)
                continue
            if i == 2:
                item.setFlags(QtCore.Qt.ItemIsEnabled)
                item.setFlags(QtCore.Qt.ItemIsSelectable)
            self.grouping_table.setItem(row_position, i, item)

    def on_add_group_button_clicked(self, slot):
        self.add_group_button.clicked.connect(slot)

    def on_remove_group_button_clicked(self, slot):
        self.remove_group_button.clicked.connect(slot)

    def on_table_data_changed(self, slot):
        self._on_table_data_changed = slot

    def _get_selected_row_indices(self):
        return list(set(index.row() for index in self.grouping_table.selectedIndexes()))

    def get_selected_group_names(self):
        indexes = self._get_selected_row_indices()
        return [str(self.grouping_table.item(i, 0).text()) for i in indexes]

    def remove_selected_groups(self):
        indices = self._get_selected_row_indices()
        for index in reversed(sorted(indices)):
            self.grouping_table.removeRow(index)

    def remove_last_row(self):
        last_row = self.grouping_table.rowCount() - 1
        if last_row >= 0:
            self.grouping_table.removeRow(last_row)

    def num_rows(self):
        return self.grouping_table.rowCount()

    def on_item_changed(self):
        """Not yet implemented."""
        pass

    def on_cell_changed(self, _row, _col):
        if not self._updating:
            self._on_table_data_changed()

    def get_table_contents(self):
        if self._updating:
            return []
        ret = [[None for _ in range(3)] for _ in range(self.num_rows())]
        for i in range(self.num_rows()):
            for j in range(3):
                ret[i][j] = str(self.grouping_table.item(i, j).text())
        return ret

    def notify_data_changed(self):
        if not self._updating:
            self.dataChanged.emit()

    def clear(self):
        # Go backwards to preserve indices
        for row in reversed(range(self.num_rows())):
            self.grouping_table.removeRow(row)

    def disable_updates(self):
        self._updating = True

    def enable_updates(self):
        self._updating = False
