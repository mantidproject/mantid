# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtGui, QtCore
from qtpy.QtCore import Signal
from qtpy.QtWidgets import QTableWidgetItem
from mantidqtinterfaces.Muon.GUI.Common import message_box
from mantidqt.utils.observer_pattern import GenericObserver

GROUP_TABLE_COLUMNS = {0: "workspace_name", 1: "run", 2: "detector", 3: "to_analyse", 4: "rebin", 5: "rebin_options"}
INVERSE_GROUP_TABLE_COLUMNS = {"workspace_name": 0, "run": 1, "detector": 2, "to_analyse": 3, "rebin": 4, "rebin_options": 5}
TABLE_COLUMN_FLAGS = {
    "workspace_name": QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled,
    "run": QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled,
    "detector": QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled,
    "to_analyse": QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled,
    "rebin": QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled,
    "rebin_options": QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled,
}


class EAGroupingTableView(QtWidgets.QWidget):
    # For use by parent widget
    dataChanged = Signal()

    def warning_popup(self, message):
        message_box.warning(str(message), parent=self)

    def __init__(self, parent=None):
        super(EAGroupingTableView, self).__init__(parent)

        self.grouping_table = QtWidgets.QTableWidget(self)
        self.set_up_table()
        self.setup_interface_layout()
        self.grouping_table.cellChanged.connect(self.on_cell_changed)

        self._validate_group_name_entry = lambda text: True
        self._on_table_data_changed = lambda: 0

        # whether the table is updating and therefore we shouldn't respond to signals
        self._updating = False
        # whether the interface should be disabled
        self._disabled = False

        self.change_once = False

        self.disable_table_observer = GenericObserver(self.disable_editing)
        self.enable_table_observer = GenericObserver(self.enable_editing)

    def setup_interface_layout(self):
        self.setObjectName("GroupingTableView")
        self.resize(500, 500)

        self.remove_group_button = QtWidgets.QToolButton()

        size_policy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(self.remove_group_button.sizePolicy().hasHeightForWidth())

        self.remove_group_button.setSizePolicy(size_policy)
        self.remove_group_button.setObjectName("removeGroupButton")
        self.remove_group_button.setToolTip("Remove selected/last group(s) from the table")
        self.remove_group_button.setText("-")

        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.remove_group_button)
        self.spacer_item = QtWidgets.QSpacerItem(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)
        self.horizontal_layout.addItem(self.spacer_item)
        self.horizontal_layout.setAlignment(QtCore.Qt.AlignLeft)

        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addWidget(self.grouping_table)
        self.vertical_layout.addLayout(self.horizontal_layout)

        self.setLayout(self.vertical_layout)

    def set_up_table(self):
        self.grouping_table.setColumnCount(6)
        self.grouping_table.setHorizontalHeaderLabels(["Workspace Name", "Run", "Detector", "Analyse (plot/fit)", "Rebin", "Rebin Options"])
        header = self.grouping_table.horizontalHeader()
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(3, QtWidgets.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(4, QtWidgets.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(5, QtWidgets.QHeaderView.ResizeToContents)
        vertical_headers = self.grouping_table.verticalHeader()
        vertical_headers.setSectionsMovable(False)
        vertical_headers.setSectionResizeMode(QtWidgets.QHeaderView.ResizeToContents)
        vertical_headers.setVisible(True)
        self.grouping_table.setColumnHidden(0, True)

        self.grouping_table.horizontalHeaderItem(1).setToolTip("The run :\n    - The run can only use digits, characters and _")
        self.grouping_table.horizontalHeaderItem(2).setToolTip("The detector :\n    - The detector can only use digits, characters and _")
        self.grouping_table.horizontalHeaderItem(3).setToolTip("Whether to include this group in the analysis.")

        self.grouping_table.horizontalHeaderItem(4).setToolTip("A list of Rebins :\n  - Select None, Fixed or Variable from the list.")
        self.grouping_table.horizontalHeaderItem(5).setToolTip(
            "Rebin Options :\n  For fixed rebin enter number of steps\n  For variable rebin enter Bin Boundaries"
        )

    def num_rows(self):
        return self.grouping_table.rowCount()

    def num_cols(self):
        return self.grouping_table.columnCount()

    def notify_data_changed(self):
        if not self._updating:
            self.dataChanged.emit()

    @staticmethod
    def get_index_of_text(selector, text):
        index = selector.findText(text)
        return index if index != -1 else 0

    # ------------------------------------------------------------------------------------------------------------------
    # Adding / removing table entries
    # ------------------------------------------------------------------------------------------------------------------
    def add_entry_to_table(self, row_entries):
        assert len(row_entries) == self.grouping_table.columnCount()
        row_position = self.grouping_table.rowCount()
        self.grouping_table.insertRow(row_position)
        self.add_row_entries(row_entries, row_position)

    def add_row_entries(self, row_entries, row_position):
        for i, entry in enumerate(row_entries):
            table_item = QtWidgets.QTableWidgetItem(entry)
            table_item.setFlags(TABLE_COLUMN_FLAGS[GROUP_TABLE_COLUMNS[i]])
            if GROUP_TABLE_COLUMNS[i] == "to_analyse":
                if entry:
                    table_item.setCheckState(QtCore.Qt.Checked)
                else:
                    table_item.setCheckState(QtCore.Qt.Unchecked)
            if GROUP_TABLE_COLUMNS[i] == "rebin":
                self.add_rebin_cell(row_position, i, table_item)

            self.grouping_table.setItem(row_position, i, table_item)

    def add_rebin_cell(self, row_position, column, table_item):
        rebin_selector_widget = self._rebin_selection_cell_widget()
        rebin_selector_widget.currentIndexChanged.connect(lambda i, row_position=row_position: self.on_rebin_combo_changed(i, row_position))
        index = int(table_item.text())
        rebin_selector_widget.setCurrentIndex(index)
        self.grouping_table.setCellWidget(row_position, column, rebin_selector_widget)

    def _get_selected_row_indices(self):
        return list(set(index.row() for index in self.grouping_table.selectedIndexes()))

    def get_selected_group_names(self):
        indexes = self._get_selected_row_indices()
        return [str(self.grouping_table.item(i, 0).text()) for i in indexes]

    def remove_selected_groups(self):
        indices = self._get_selected_row_indices()
        for index in sorted(indices, reverse=True):
            self.grouping_table.removeRow(index)

    def remove_last_row(self):
        last_row = self.grouping_table.rowCount() - 1
        if last_row >= 0:
            self.grouping_table.removeRow(last_row)

    def enter_group_name(self):
        new_group_name, ok = QtWidgets.QInputDialog.getText(self, "Detector", "Enter name of new detector:")
        if ok:
            return new_group_name

    def _rebin_selection_cell_widget(self):
        # The widget for the group selection columns
        selector = QtWidgets.QComboBox(self)
        selector.setToolTip("Select a rebin option")
        selector.addItems(["None", "Fixed", "Variable"])
        return selector

    def contextMenuEvent(self, _event):
        """Overridden method"""
        self.menu = QtWidgets.QMenu(self)

        self.remove_group_action = self._context_menu_remove_group_action(self.remove_group_button.clicked.emit)

        if self._disabled:
            self.remove_group_action.setEnabled(False)

        self.menu.addAction(self.remove_group_action)

        self.menu.popup(QtGui.QCursor.pos())

    # ------------------------------------------------------------------------------------------------------------------
    # Context menu on right-click in the table
    # ------------------------------------------------------------------------------------------------------------------

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

    # ------------------------------------------------------------------------------------------------------------------
    # Slot connections
    # ------------------------------------------------------------------------------------------------------------------

    def on_user_changes_group_name(self, slot):
        self._validate_group_name_entry = slot

    def on_remove_group_button_clicked(self, slot):
        self.remove_group_button.clicked.connect(slot)

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
        item = self.get_table_item(row, INVERSE_GROUP_TABLE_COLUMNS["to_analyse"])
        item.setCheckState(checked_state)

    def set_to_analyse_state_quietly(self, row, state):
        checked_state = QtCore.Qt.Checked if state is True else QtCore.Qt.Unchecked
        item = self.get_table_item(row, INVERSE_GROUP_TABLE_COLUMNS["to_analyse"])
        self.grouping_table.blockSignals(True)
        item.setCheckState(checked_state)
        self.grouping_table.blockSignals(False)

    def get_table_contents(self):
        if self._updating:
            return []
        table_by_row = []
        for row in range(self.num_rows()):
            row_list = []
            for col in range(self.num_cols()):
                row_list.append(str(self.grouping_table.item(row, col).text()))
            table_by_row.append(row_list)
        return table_by_row

    def clear(self):
        # Go backwards to preserve indices
        for row in reversed(range(self.num_rows())):
            self.grouping_table.removeRow(row)

    def on_rebin_combo_changed(self, index, row):
        self.change_once = False
        self.grouping_table.setItem(row, 4, QTableWidgetItem(str(index)))

    def rebin_fixed_chosen(self, row):
        steps, ok = QtWidgets.QInputDialog.getText(
            self, "Steps", "Rebinning creates a new workspace.\nEnter the new bin width in KeV for a new workspace:"
        )
        if not ok:
            self.grouping_table.cellWidget(row, 4).setCurrentIndex(0)
            return
        if not steps.strip():
            self.warning_popup("Rebin parameters not given")
            self.grouping_table.cellWidget(row, 4).setCurrentIndex(0)
            return
        try:
            steps = float(steps)
        except ValueError:
            self.grouping_table.cellWidget(row, 4).setCurrentIndex(0)
            self.warning_popup("Given rebin step is invalid")
            return
        steps_text = "Steps: " + str(steps) + " KeV"
        table_item = QTableWidgetItem(steps_text)
        table_item.setFlags(TABLE_COLUMN_FLAGS["rebin_options"])
        self.grouping_table.setItem(row, 5, table_item)

    def rebin_variable_chosen(self, row):
        steps, ok = QtWidgets.QInputDialog.getText(
            self,
            "Bin Boundaries",
            "Rebinning creates a new workspace.\n"
            "A comma separated list of first bin boundary, width, last bin "
            "boundary.\n"
            "Optionally this can be followed by a comma and more widths and last"
            " boundary pairs.\n"
            "Optionally this can also be a single number, which is the bin "
            "width.\n"
            "Negative width values indicate logarithmic binning.\n\n"
            "For example:\n"
            "2,-0.035,10: from 2 rebin in Logarithmic bins of 0.035 up to 10;\n"
            "0,100,10000,200,20000: from 0 rebin in steps of 100 to 10,000 then "
            "steps of 200 to 20,000",
        )
        if not ok:
            self.grouping_table.cellWidget(row, 4).setCurrentIndex(0)
            return
        if not steps.strip():
            self.grouping_table.cellWidget(row, 4).setCurrentIndex(0)
            self.warning_popup("Rebin parameters not given")
            return
        bin_text = "Bin Boundaries: " + str(steps)
        table_item = QTableWidgetItem(bin_text)
        table_item.setFlags(TABLE_COLUMN_FLAGS["rebin_options"])
        self.grouping_table.setItem(row, 5, table_item)

    def rebin_none_chosen(self, row):
        table_item = QTableWidgetItem("")
        table_item.setFlags(TABLE_COLUMN_FLAGS["rebin_options"])
        self.grouping_table.setItem(row, 5, table_item)

    # ------------------------------------------------------------------------------------------------------------------
    # Enabling and disabling editing and updating of the widget
    # ------------------------------------------------------------------------------------------------------------------

    def disable_updates(self):
        self._updating = True

    def enable_updates(self):
        self._updating = False

    def disable_editing(self):
        self.disable_updates()
        self._disabled = True
        self._disable_all_table_items()
        self.enable_updates()

    def enable_editing(self):
        self.disable_updates()
        self._disabled = False
        self._enable_all_table_items()
        self.enable_updates()

    def _enable_buttons(self):
        self.remove_group_button.setEnabled(True)

    def _disable_buttons(self):
        self.remove_group_button.setEnabled(False)

    def _disable_all_table_items(self):
        self.grouping_table.setEnabled(False)

    def _enable_all_table_items(self):
        self.grouping_table.setEnabled(True)
