# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore, QtGui
from qtpy.QtCore import Signal

from mantidqtinterfaces.Muon.GUI.Common import message_box
from mantidqtinterfaces.Muon.GUI.Common.utilities import table_utils

diff_columns = {0: "diff_name", 1: "to_analyse", 2: "group_1", 3: "group_2"}
inverse_diff_columns = {"diff_name": 0, "to_analyse": 1, "group_1": 2, "group_2": 3}


class DifferenceTableView(QtWidgets.QWidget):
    dataChanged = Signal()

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super(DifferenceTableView, self).__init__(parent)

        self.diff_table = QtWidgets.QTableWidget(self)
        self.set_up_table()
        self.setup_interface_layout()
        self.diff_table.itemChanged.connect(self.on_item_changed)
        self.diff_table.cellChanged.connect(self.on_cell_changed)

        # Table entry validation
        self._validate_diff_name_entry = lambda text: True

        self._on_table_data_changed = lambda row, column: 0

        # The active groups that can be selected from the group combo box
        self._group_selections = []

        # whether the table is updating and therefore we shouldn't respond to signals
        self._updating = False

        # the right-click context menu
        self.menu = None
        self._disabled = False
        self.add_diff_action = None
        self.remove_diff_action = None

    def setup_interface_layout(self):
        self.setObjectName("diffTableView")
        self.resize(500, 500)

        self.add_diff_button = QtWidgets.QToolButton()
        self.remove_diff_button = QtWidgets.QToolButton()

        size_policy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(self.add_diff_button.sizePolicy().hasHeightForWidth())
        size_policy.setHeightForWidth(self.remove_diff_button.sizePolicy().hasHeightForWidth())

        self.add_diff_button.setSizePolicy(size_policy)
        self.add_diff_button.setObjectName("addGroupButton")
        self.add_diff_button.setToolTip("Add a diff to the end of the table")
        self.add_diff_button.setText("+")

        self.remove_diff_button.setSizePolicy(size_policy)
        self.remove_diff_button.setObjectName("removeGroupButton")
        self.remove_diff_button.setToolTip("Remove selected/last diff(s) from the table")
        self.remove_diff_button.setText("-")

        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.add_diff_button)
        self.horizontal_layout.addWidget(self.remove_diff_button)
        self.spacer_item = QtWidgets.QSpacerItem(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)
        self.horizontal_layout.addItem(self.spacer_item)
        self.horizontal_layout.setAlignment(QtCore.Qt.AlignLeft)

        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addWidget(self.diff_table)
        self.vertical_layout.addLayout(self.horizontal_layout)

        self.setLayout(self.vertical_layout)

    def set_up_table(self):
        self.diff_table.setColumnCount(4)
        vertical_headers = self.diff_table.verticalHeader()
        vertical_headers.setSectionsMovable(False)
        vertical_headers.setSectionResizeMode(QtWidgets.QHeaderView.ResizeToContents)
        vertical_headers.setVisible(True)

    def num_rows(self):
        return self.diff_table.rowCount()

    def num_cols(self):
        return self.diff_table.columnCount()

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
            self.diff_table.removeRow(row)

    def notify_data_changed(self):
        if not self._updating:
            self.dataChanged.emit()

    def add_entry_to_table(self, row_entries, color=(255, 255, 255), tooltip=""):
        assert len(row_entries) == self.diff_table.columnCount()
        q_color = QtGui.QColor(*color, alpha=127)
        q_brush = QtGui.QBrush(q_color)

        row_position = self.diff_table.rowCount()
        self.diff_table.insertRow(row_position)
        for i, entry in enumerate(row_entries):
            item = QtWidgets.QTableWidgetItem(entry)
            item.setBackground(q_brush)
            item.setToolTip(tooltip)
            if diff_columns[i] == "diff_name":
                diff_name_widget = table_utils.ValidatedTableItem(self._validate_diff_name_entry)
                diff_name_widget.setText(entry)
                self.diff_table.setItem(row_position, i, diff_name_widget)
                item.setFlags(QtCore.Qt.ItemIsEnabled)
                item.setFlags(QtCore.Qt.ItemIsSelectable)
            if diff_columns[i] == "group_1":
                group1_selector_widget = self._group_selection_cell_widget()
                # ensure changing the selection sends an update signal
                group1_selector_widget.currentIndexChanged.connect(lambda: self.on_cell_changed(row_position, 2))
                index = self.get_index_of_text(group1_selector_widget, entry)
                group1_selector_widget.setCurrentIndex(index)
                self.diff_table.setCellWidget(row_position, i, group1_selector_widget)
            if diff_columns[i] == "group_2":
                group2_selector_widget = self._group_selection_cell_widget()
                # ensure changing the selection sends an update signal
                group2_selector_widget.currentIndexChanged.connect(lambda: self.on_cell_changed(row_position, 3))
                index = self.get_index_of_text(group2_selector_widget, entry)
                group2_selector_widget.setCurrentIndex(index)
                self.diff_table.setCellWidget(row_position, i, group2_selector_widget)
            if diff_columns[i] == "to_analyse":
                if entry:
                    item.setCheckState(QtCore.Qt.Checked)
                else:
                    item.setCheckState(QtCore.Qt.Unchecked)
            self.diff_table.setItem(row_position, i, item)

    def _group_selection_cell_widget(self):
        # The widget for the group selection columns
        selector = QtWidgets.QComboBox(self)
        selector.setToolTip("Select a group from the grouping table")
        selector.addItems(self._group_selections)
        return selector

    def get_table_contents(self):
        if self._updating:
            return []
        ret = [[None for _ in range(self.num_cols())] for _ in range(self.num_rows())]
        for row in range(self.num_rows()):
            for col in range(self.num_cols()):
                if diff_columns[col] == "group_1" or diff_columns[col] == "group_2":
                    # columns with widgets
                    ret[row][col] = str(self.diff_table.cellWidget(row, col).currentText())
                else:
                    # columns without widgets
                    ret[row][col] = str(self.diff_table.item(row, col).text())
        return ret

    def get_table_item(self, row, col):
        return self.diff_table.item(row, col)

    def get_table_item_text(self, row, col):
        if diff_columns[col] == "group_1" or diff_columns[col] == "group_2":
            return str(self.diff_table.cellWidget(row, col).currentText())
        else:  # columns without widgets
            return str(self.diff_table.item(row, col).text())

    # ------------------------------------------------------------------------------------------------------------------
    # Signal / Slot connections
    # ------------------------------------------------------------------------------------------------------------------

    def on_user_changes_diff_name(self, slot):
        self._validate_diff_name_entry = slot

    def on_add_diff_button_clicked(self, slot):
        self.add_diff_button.clicked.connect(slot)

    def on_remove_diff_button_clicked(self, slot):
        self.remove_diff_button.clicked.connect(slot)

    def on_table_data_changed(self, slot):
        self._on_table_data_changed = slot

    def on_item_changed(self):
        """Not yet implemented."""
        if not self._updating:
            pass

    def on_cell_changed(self, row, column):
        if not self._updating:
            if not (isinstance(self.sender(), QtWidgets.QTableWidget) or self.sender().pos().isNull()):
                pos_index = self.diff_table.indexAt(self.sender().pos())
                row = pos_index.row()
                column = pos_index.column()
            self._on_table_data_changed(row, column)

    # ------------------------------------------------------------------------------------------------------------------
    # Context Menu
    # ------------------------------------------------------------------------------------------------------------------

    def contextMenuEvent(self, _event):
        """Overridden method for dealing with the right-click context menu"""
        self.menu = QtWidgets.QMenu(self)

        self.add_diff_action = self._context_menu_add_diff_action(self.add_diff_button.clicked.emit)
        self.remove_diff_action = self._context_menu_remove_diff_action(self.remove_diff_button.clicked.emit)

        if self._disabled:
            self.add_diff_action.setEnabled(False)
            self.remove_diff_action.setEnabled(False)
        # set-up the menu
        self.menu.addAction(self.add_diff_action)
        self.menu.addAction(self.remove_diff_action)
        self.menu.popup(QtGui.QCursor.pos())

    def _context_menu_add_diff_action(self, slot):
        add_diff_action = QtWidgets.QAction("Add diff", self)
        add_diff_action.setCheckable(False)
        if len(self._get_selected_row_indices()) > 0:
            add_diff_action.setEnabled(False)
        add_diff_action.triggered.connect(slot)
        return add_diff_action

    def _context_menu_remove_diff_action(self, slot):
        if len(self._get_selected_row_indices()) > 1:
            # use plural if >1 item selected
            remove_diff_action = QtWidgets.QAction("Remove diffs", self)
        else:
            remove_diff_action = QtWidgets.QAction("Remove diff", self)
        if self.num_rows() == 0:
            remove_diff_action.setEnabled(False)
        remove_diff_action.triggered.connect(slot)
        return remove_diff_action

    # ------------------------------------------------------------------------------------------------------------------
    # Adding / Removing diffs
    # ------------------------------------------------------------------------------------------------------------------

    def _get_selected_row_indices(self):
        return list(set(index.row() for index in self.diff_table.selectedIndexes()))

    def get_selected_diff_names(self):
        indexes = self._get_selected_row_indices()
        return [str(self.diff_table.item(i, 0).text()) for i in indexes]

    def remove_selected_diffs(self):
        indices = self._get_selected_row_indices()
        for index in reversed(sorted(indices)):
            self.diff_table.removeRow(index)

    def remove_last_row(self):
        last_row = self.diff_table.rowCount() - 1
        if last_row >= 0:
            self.diff_table.removeRow(last_row)

    def enter_diff_name(self):
        new_diff_name, ok = QtWidgets.QInputDialog.getText(self, "diff Name", "Enter name of new diff:")
        if ok:
            return new_diff_name

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
                column_name = diff_columns[col]
                if column_name == "group_1" or column_name == "group_2":
                    item = self.diff_table.cellWidget(row, col)
                    item.setEnabled(False)
                else:
                    item = self.diff_table.item(row, col)
                    item.setFlags(QtCore.Qt.ItemIsSelectable)

    def _enable_all_table_items(self):
        for row in range(self.num_rows()):
            for col in range(self.num_cols()):
                column_name = diff_columns[col]
                if column_name == "group_1" or column_name == "group_2":
                    item = self.diff_table.cellWidget(row, col)
                    item.setEnabled(True)
                elif column_name == "diff_name":
                    item = self.diff_table.item(row, col)
                    item.setFlags(QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled)
                elif column_name == "to_analyse":
                    item = self.diff_table.item(row, col)
                    item.setFlags(QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled)
                else:
                    return

    def _enable_all_buttons(self):
        self.add_diff_button.setEnabled(True)
        self.remove_diff_button.setEnabled(True)

    def _disable_all_buttons(self):
        self.add_diff_button.setEnabled(False)
        self.remove_diff_button.setEnabled(False)

    def set_to_analyse_state(self, row, state):
        checked_state = QtCore.Qt.Checked if state is True else QtCore.Qt.Unchecked
        item = self.get_table_item(row, inverse_diff_columns["to_analyse"])
        item.setCheckState(checked_state)

    def set_to_analyse_state_quietly(self, row, state):
        checked_state = QtCore.Qt.Checked if state is True else QtCore.Qt.Unchecked
        item = self.get_table_item(row, 1)
        self.diff_table.blockSignals(True)
        item.setCheckState(checked_state)
        self.diff_table.blockSignals(False)

    def set_table_headers_pairs(self):
        self.diff_table.setHorizontalHeaderLabels(["Diff Name", "Analyse (plot/fit)", "Pair 1", "Pair 2"])

        header = self.diff_table.horizontalHeader()
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(3, QtWidgets.QHeaderView.Stretch)

        self.diff_table.horizontalHeaderItem(0).setToolTip(
            "The name of the diff :"
            "\n    - The name must be unique across all groups/diffs"
            "\n    - The name can only use digits, characters and _"
        )
        self.diff_table.horizontalHeaderItem(2).setToolTip("Pair 1 of the diff, selected from the pair table")
        self.diff_table.horizontalHeaderItem(3).setToolTip("Pair 2 of the diff, selected from the pair table")
        self.diff_table.horizontalHeaderItem(1).setToolTip("Whether to include this diff in the analysis")

    def set_table_headers_groups(self):
        self.diff_table.setHorizontalHeaderLabels(["Diff Name", "Analyse (plot/fit)", "Group 1", "Group 2"])

        header = self.diff_table.horizontalHeader()
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(3, QtWidgets.QHeaderView.Stretch)

        self.diff_table.horizontalHeaderItem(0).setToolTip(
            "The name of the diff :"
            "\n    - The name must be unique across all groups/diffs"
            "\n    - The name can only use digits, characters and _"
        )
        self.diff_table.horizontalHeaderItem(2).setToolTip("Group 1 of the diff, selected from the group table")
        self.diff_table.horizontalHeaderItem(3).setToolTip("Group 2 of the diff, selected from the group table")
        self.diff_table.horizontalHeaderItem(1).setToolTip("Whether to include this diff in the analysis")
