# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtGui, QtCore
from qtpy.QtCore import Signal
import sys
from mantidqtinterfaces.Muon.GUI.Common import message_box

group_table_columns = {0: "group_name", 2: "to_analyse", 3: "detector_ids", 4: "number_of_detectors", 1: "periods"}
inverse_group_table_columns = {"group_name": 0, "to_analyse": 2, "detector_ids": 3, "number_of_detectors": 4, "periods": 1}
table_column_flags = {
    "group_name": QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled,
    "to_analyse": QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled,
    "detector_ids": QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsEditable,
    "number_of_detectors": QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled,
    "periods": QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsEditable,
}


class GroupingTableView(QtWidgets.QWidget):
    # For use by parent widget
    dataChanged = Signal()
    addPairRequested = Signal(str, str)

    def warning_popup(self, message):
        message_box.warning(str(message), parent=self)

    def __init__(self, parent=None):
        super(GroupingTableView, self).__init__(parent)

        self.grouping_table = QtWidgets.QTableWidget(self)
        self.set_up_table()

        self.setup_interface_layout()

        self.grouping_table.cellChanged.connect(self.on_cell_changed)

        self._validate_group_name_entry = lambda text: True
        self._validate_detector_ID_entry = lambda text: True
        self._on_table_data_changed = lambda: 0

        # whether the table is updating and therefore we shouldn't respond to signals
        self._updating = False
        # whether the interface should be disabled
        self._disabled = False

    def setup_interface_layout(self):
        self.setObjectName("GroupingTableView")
        self.resize(500, 500)

        self.add_group_button = QtWidgets.QToolButton()
        self.remove_group_button = QtWidgets.QToolButton()

        self.group_range_label = QtWidgets.QLabel()
        self.group_range_label.setText("Group Asymmetry Range from:")
        self.group_range_min = QtWidgets.QLineEdit("0.0")
        self.group_range_min.setEnabled(False)
        positive_float_validator = QtGui.QDoubleValidator(0.0, sys.float_info.max, 5)
        self.group_range_min.setValidator(positive_float_validator)

        self.group_range_use_first_good_data = QtWidgets.QCheckBox()
        self.group_range_use_first_good_data.setText("\u03bcs (From data file)")

        self.group_range_use_first_good_data.setChecked(True)
        self.group_range_max = QtWidgets.QLineEdit("1.0")
        self.group_range_max.setEnabled(False)
        self.group_range_max.setValidator(positive_float_validator)

        self.group_range_use_last_data = QtWidgets.QCheckBox()
        self.group_range_use_last_data.setText("\u03bcs (From data file)")
        self.group_range_use_last_data.setChecked(True)
        self.group_range_to_label = QtWidgets.QLabel()
        self.group_range_to_label.setText("to:")

        self.group_range_layout = QtWidgets.QGridLayout()
        self.group_range_layout_min = QtWidgets.QHBoxLayout()
        self.group_range_layout.addWidget(self.group_range_label, 0, 0)
        self.group_range_layout.addWidget(self.group_range_min, 0, 1)
        self.group_range_layout.addWidget(self.group_range_use_first_good_data, 0, 2)

        self.group_range_layout_max = QtWidgets.QHBoxLayout()
        self.group_range_layout.addWidget(self.group_range_to_label, 1, 0, QtCore.Qt.AlignRight)
        self.group_range_layout.addWidget(self.group_range_max, 1, 1)
        self.group_range_layout.addWidget(self.group_range_use_last_data, 1, 2)

        size_policy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(self.add_group_button.sizePolicy().hasHeightForWidth())
        size_policy.setHeightForWidth(self.remove_group_button.sizePolicy().hasHeightForWidth())

        self.add_group_button.setSizePolicy(size_policy)
        self.add_group_button.setObjectName("addGroupButton")
        self.add_group_button.setToolTip("Add a group to the end of the table")
        self.add_group_button.setText("+")

        self.remove_group_button.setSizePolicy(size_policy)
        self.remove_group_button.setObjectName("removeGroupButton")
        self.remove_group_button.setToolTip("Remove selected/last group(s) from the table")
        self.remove_group_button.setText("-")

        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.add_group_button)
        self.horizontal_layout.addWidget(self.remove_group_button)
        self.spacer_item = QtWidgets.QSpacerItem(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)
        self.horizontal_layout.addItem(self.spacer_item)
        self.horizontal_layout.setAlignment(QtCore.Qt.AlignLeft)

        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addWidget(self.grouping_table)
        self.vertical_layout.addLayout(self.horizontal_layout)
        self.vertical_layout.addLayout(self.group_range_layout)

        self.setLayout(self.vertical_layout)

    def set_up_table(self):
        self.grouping_table.setColumnCount(5)
        self.grouping_table.setHorizontalHeaderLabels(["Group Name", "Periods", "Analyse (plot/fit)", "Detector IDs", "N Detectors"])
        header = self.grouping_table.horizontalHeader()
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(2, QtWidgets.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(3, QtWidgets.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(4, QtWidgets.QHeaderView.Stretch)
        vertical_headers = self.grouping_table.verticalHeader()
        vertical_headers.setSectionsMovable(False)
        vertical_headers.setSectionResizeMode(QtWidgets.QHeaderView.ResizeToContents)
        vertical_headers.setVisible(True)

        self.grouping_table.horizontalHeaderItem(0).setToolTip(
            "The name of the group :"
            "\n    - The name must be unique across all groups/pairs"
            "\n    - The name can only use digits, characters and _"
        )
        self.grouping_table.horizontalHeaderItem(1).setToolTip("Sum of periods to use when calculating this group.")
        self.grouping_table.horizontalHeaderItem(2).setToolTip("Whether to include this group in the analysis.")

        self.grouping_table.horizontalHeaderItem(3).setToolTip(
            "The sorted list of detectors :"
            "\n  - The list can only contain integers."
            "\n  - , is used to separate detectors or ranges."
            '\n  - "-" denotes a range, i,e "1-5" is the same as'
            ' "1,2,3,4,5" '
        )
        self.grouping_table.horizontalHeaderItem(4).setToolTip("The number of detectors in the group.")

    def num_rows(self):
        return self.grouping_table.rowCount()

    def num_cols(self):
        return self.grouping_table.columnCount()

    def notify_data_changed(self):
        if not self._updating:
            self.dataChanged.emit()

    # ------------------------------------------------------------------------------------------------------------------
    # Adding / removing table entries
    # ------------------------------------------------------------------------------------------------------------------
    def add_entry_to_table(self, row_entries, color=(255, 255, 255), tooltip=""):
        assert len(row_entries) == self.grouping_table.columnCount()
        row_position = self.grouping_table.rowCount()
        self.grouping_table.insertRow(row_position)
        q_color = QtGui.QColor(*color, alpha=127)
        q_brush = QtGui.QBrush(q_color)
        for i, entry in enumerate(row_entries):
            table_item = QtWidgets.QTableWidgetItem(entry)
            table_item.setBackground(q_brush)
            table_item.setToolTip(tooltip)
            self.grouping_table.setItem(row_position, i, table_item)
            table_item.setFlags(table_column_flags[group_table_columns[i]])

            if group_table_columns[i] == "to_analyse":
                if entry:
                    table_item.setCheckState(QtCore.Qt.Checked)
                else:
                    table_item.setCheckState(QtCore.Qt.Unchecked)

    def _get_selected_row_indices(self):
        return list(set(index.row() for index in self.grouping_table.selectedIndexes()))

    def get_selected_group_names_and_indexes(self):
        indexes = self._get_selected_row_indices()
        return [[str(self.grouping_table.item(i, 0).text()), i] for i in indexes]

    def remove_group_by_index(self, index):
        self.grouping_table.removeRow(index)

    def remove_last_row(self):
        last_row = self.grouping_table.rowCount() - 1
        if last_row >= 0:
            self.grouping_table.removeRow(last_row)

    def enter_group_name(self):
        new_group_name, ok = QtWidgets.QInputDialog.getText(self, "Group Name", "Enter name of new group:")
        if ok:
            return new_group_name

    # ------------------------------------------------------------------------------------------------------------------
    # Context menu on right-click in the table
    # ------------------------------------------------------------------------------------------------------------------

    def _context_menu_add_group_action(self, slot):
        add_group_action = QtWidgets.QAction("Add Group", self)
        if len(self._get_selected_row_indices()) > 0:
            add_group_action.setEnabled(False)
        add_group_action.triggered.connect(slot)
        return add_group_action

    def _context_menu_remove_group_action(self, slot):
        if len(self._get_selected_row_indices()) > 1:
            # use plural if >1 item selected
            remove_group_action = QtWidgets.QAction("Remove Groups", self)
        else:
            remove_group_action = QtWidgets.QAction("Remove Group", self)
        if self.num_rows() == 0:
            remove_group_action.setEnabled(False)
        remove_group_action.triggered.connect(slot)
        return remove_group_action

    def _context_menu_add_pair_action(self, slot):
        add_pair_action = QtWidgets.QAction("Add Pair", self)
        if len(self._get_selected_row_indices()) != 2:
            add_pair_action.setEnabled(False)
        add_pair_action.triggered.connect(slot)
        return add_pair_action

    def contextMenuEvent(self, _event):
        """Overridden method"""
        self.menu = QtWidgets.QMenu(self)

        self.add_group_action = self._context_menu_add_group_action(self.add_group_button.clicked.emit)
        self.remove_group_action = self._context_menu_remove_group_action(self.remove_group_button.clicked.emit)
        self.add_pair_action = self._context_menu_add_pair_action(self.add_pair_requested)

        if self._disabled:
            self.add_group_action.setEnabled(False)
            self.remove_group_action.setEnabled(False)
            self.add_pair_action.setEnabled(False)

        self.menu.addAction(self.add_group_action)
        self.menu.addAction(self.remove_group_action)
        self.menu.addAction(self.add_pair_action)

        self.menu.popup(QtGui.QCursor.pos())

    # ------------------------------------------------------------------------------------------------------------------
    # Slot connections
    # ------------------------------------------------------------------------------------------------------------------

    def on_user_changes_group_name(self, slot):
        self._validate_group_name_entry = slot

    def on_user_changes_detector_IDs(self, slot):
        self._validate_detector_ID_entry = slot

    def on_add_group_button_clicked(self, slot):
        self.add_group_button.clicked.connect(slot)

    def on_remove_group_button_clicked(self, slot):
        self.remove_group_button.clicked.connect(slot)

    def on_table_data_changed(self, slot):
        self._on_table_data_changed = slot

    def add_pair_requested(self):
        selected_names = self.get_selected_group_names_and_indexes()
        self.addPairRequested.emit(selected_names[0][0], selected_names[1][0])

    def on_cell_changed(self, _row, _col):
        if not self._updating:
            self._on_table_data_changed(_row, _col)

    def on_user_changes_min_range_source(self, slot):
        self.group_range_use_first_good_data.stateChanged.connect(slot)

    def on_user_changes_max_range_source(self, slot):
        self.group_range_use_last_data.stateChanged.connect(slot)

    def on_user_changes_group_range_min_text_edit(self, slot):
        self.group_range_min.editingFinished.connect(slot)

    def on_user_changes_group_range_max_text_edit(self, slot):
        self.group_range_max.editingFinished.connect(slot)

    # ------------------------------------------------------------------------------------------------------------------
    #
    # ------------------------------------------------------------------------------------------------------------------

    def get_table_item_text(self, row, col):
        return self.grouping_table.item(row, col).text()

    def get_table_item(self, row, col):
        return self.grouping_table.item(row, col)

    def set_to_analyse_state(self, row, state):
        checked_state = QtCore.Qt.Checked if state is True else QtCore.Qt.Unchecked
        item = self.get_table_item(row, inverse_group_table_columns["to_analyse"])
        item.setCheckState(checked_state)

    def set_to_analyse_state_quietly(self, row, state):
        checked_state = QtCore.Qt.Checked if state is True else QtCore.Qt.Unchecked
        item = self.get_table_item(row, inverse_group_table_columns["to_analyse"])
        self.grouping_table.blockSignals(True)
        item.setCheckState(checked_state)
        self.grouping_table.blockSignals(False)

    def get_table_contents(self):
        if self._updating:
            return []
        ret = []
        for row in range(self.num_rows()):
            row_list = []
            for col in range(self.num_cols()):
                row_list.append(str(self.grouping_table.item(row, col).text()))
            ret.append(row_list)
        return ret

    def clear(self):
        # Go backwards to preserve indices
        for row in reversed(range(self.num_rows())):
            self.grouping_table.removeRow(row)

    # ------------------------------------------------------------------------------------------------------------------
    # Enabling and disabling editing and updating of the widget
    # ------------------------------------------------------------------------------------------------------------------

    def disable_updates(self):
        """Usage :"""
        self._updating = True

    def enable_updates(self):
        """Usage :"""
        self._updating = False

    def disable_editing(self):
        self.disable_updates()
        self._disabled = True
        self._disable_buttons()
        self._disable_all_table_items()
        self._disable_group_ranges()
        self.enable_updates()

    def enable_editing(self):
        self.disable_updates()
        self._disabled = False
        self._enable_buttons()
        self._enable_all_table_items()
        self._enable_group_ranges()
        self.enable_updates()

    def _enable_group_ranges(self):
        self.group_range_use_first_good_data.setEnabled(True)
        self.group_range_use_last_data.setEnabled(True)

        if not self.group_range_use_first_good_data.isChecked():
            self.group_range_min.setEnabled(True)

        if not self.group_range_use_last_data.isChecked():
            self.group_range_max.setEnabled(True)

    def _disable_group_ranges(self):
        self.group_range_use_first_good_data.setEnabled(False)
        self.group_range_use_last_data.setEnabled(False)
        self.group_range_min.setEnabled(False)
        self.group_range_max.setEnabled(False)

    def _enable_buttons(self):
        self.add_group_button.setEnabled(True)
        self.remove_group_button.setEnabled(True)

    def _disable_buttons(self):
        self.add_group_button.setEnabled(False)
        self.remove_group_button.setEnabled(False)

    def _disable_all_table_items(self):
        self.grouping_table.setEnabled(False)

    def _enable_all_table_items(self):
        self.grouping_table.setEnabled(True)

    def get_group_range(self):
        return str(self.group_range_min.text()), str(self.group_range_max.text())

    def set_group_range(self, range):
        self.group_range_min.setText(range[0])
        self.group_range_max.setText(range[1])
