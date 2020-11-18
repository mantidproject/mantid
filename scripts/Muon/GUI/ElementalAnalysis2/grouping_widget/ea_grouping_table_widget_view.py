# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtGui, QtCore
from qtpy.QtCore import Signal
from Muon.GUI.Common import message_box

group_table_columns = {0: 'group_name', 2: 'to_analyse', 3: 'detector_ids', 4: 'number_of_detectors', 1: 'periods'}
inverse_group_table_columns = {'group_name': 0, 'to_analyse': 2,  'detector_ids': 3,  'number_of_detectors': 4,
                               'periods': 1}
table_column_flags = {'group_name': QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled,
                      'to_analyse': QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled,
                      'detector_ids': QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsEditable,
                      'number_of_detectors': QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled,
                      'periods': QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsEditable}


class EAGroupingTableView(QtWidgets.QWidget):
    # For use by parent widget
    dataChanged = Signal()
    addPairRequested = Signal(str, str)

    def warning_popup(self, message):
        message_box.warning(str(message), parent=self)

    def __init__(self, parent=None):
        super(EAGroupingTableView, self).__init__(parent)

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

        size_policy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)

        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.spacer_item = QtWidgets.QSpacerItem(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)
        self.horizontal_layout.addItem(self.spacer_item)
        self.horizontal_layout.setAlignment(QtCore.Qt.AlignLeft)

        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addWidget(self.grouping_table)
        self.vertical_layout.addLayout(self.horizontal_layout)

        self.setLayout(self.vertical_layout)

    def set_up_table(self):
        self.grouping_table.setColumnCount(5)
        self.grouping_table.setHorizontalHeaderLabels(["Group Name", "Periods", "Analyse (plot/fit)", "Detector IDs",
                                                       "N Detectors"])
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

        self.grouping_table.horizontalHeaderItem(0).setToolTip("The name of the group :"
                                                               "\n    - The name must be unique across all groups/pairs"
                                                               "\n    - The name can only use digits, characters and _")
        self.grouping_table.horizontalHeaderItem(1).setToolTip("Periods to use when calculating this group.")
        self.grouping_table.horizontalHeaderItem(2).setToolTip("Whether to include this group in the analysis.")

        self.grouping_table.horizontalHeaderItem(3).setToolTip("The sorted list of detectors :"
                                                               "\n  - The list can only contain integers."
                                                               "\n  - , is used to separate detectors or ranges."
                                                               "\n  - \"-\" denotes a range, i,e \"1-5\" is the same as"
                                                               " \"1,2,3,4,5\" ")
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
    def add_entry_to_table(self, row_entries, color=(255, 255, 255), tooltip=''):
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

            if group_table_columns[i] == 'to_analyse':
                if entry:
                    table_item.setCheckState(QtCore.Qt.Checked)
                else:
                    table_item.setCheckState(QtCore.Qt.Unchecked)

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

    def enter_group_name(self):
        new_group_name, ok = QtWidgets.QInputDialog.getText(self, 'Group Name', 'Enter name of new group:')
        if ok:
            return new_group_name

    # ------------------------------------------------------------------------------------------------------------------
    # Context menu on right-click in the table
    # ------------------------------------------------------------------------------------------------------------------

    def _context_menu_add_group_action(self, slot):
        add_group_action = QtWidgets.QAction('Add Group', self)
        if len(self._get_selected_row_indices()) > 0:
            add_group_action.setEnabled(False)
        add_group_action.triggered.connect(slot)
        return add_group_action

    def _context_menu_remove_group_action(self, slot):
        if len(self._get_selected_row_indices()) > 1:
            # use plural if >1 item selected
            remove_group_action = QtWidgets.QAction('Remove Groups', self)
        else:
            remove_group_action = QtWidgets.QAction('Remove Group', self)
        if self.num_rows() == 0:
            remove_group_action.setEnabled(False)
        remove_group_action.triggered.connect(slot)
        return remove_group_action

    # ------------------------------------------------------------------------------------------------------------------
    # Slot connections
    # ------------------------------------------------------------------------------------------------------------------

    def on_user_changes_group_name(self, slot):
        self._validate_group_name_entry = slot

    def on_user_changes_detector_IDs(self, slot):
        self._validate_detector_ID_entry = slot

    def on_table_data_changed(self, slot):
        self._on_table_data_changed = slot

    def on_cell_changed(self, _row, _col):
        if not self._updating:
            self._on_table_data_changed(_row, _col)

    # ------------------------------------------------------------------------------------------------------------------
    #
    # ------------------------------------------------------------------------------------------------------------------

    def get_table_item_text(self, row, col):
        return self.grouping_table.item(row, col).text()

    def get_table_item(self, row, col):
        return self.grouping_table.item(row, col)

    def set_to_analyse_state(self, row, state):
        checked_state = QtCore.Qt.Checked if state is True else QtCore.Qt.Unchecked
        item = self.get_table_item(row, inverse_group_table_columns['to_analyse'])
        item.setCheckState(checked_state)

    def set_to_analyse_state_quietly(self, row, state):
        checked_state = QtCore.Qt.Checked if state is True else QtCore.Qt.Unchecked
        item = self.get_table_item(row, inverse_group_table_columns['to_analyse'])
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
        """Usage : """
        self._updating = True

    def enable_updates(self):
        """Usage : """
        self._updating = False

    def disable_editing(self):
        self.disable_updates()
        self._disabled = True
        self._disable_buttons()
        self._disable_all_table_items()
        self.enable_updates()

    def enable_editing(self):
        self.disable_updates()
        self._disabled = False
        self._enable_buttons()
        self._enable_all_table_items()
        self.enable_updates()

    def _disable_all_table_items(self):
        self.grouping_table.setEnabled(False)

    def _enable_all_table_items(self):
        self.grouping_table.setEnabled(True)
