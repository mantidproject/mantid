# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore
from mantidqtinterfaces.Muon.GUI.Common.utilities import table_utils
from mantidqtinterfaces.Muon.GUI.Common.message_box import warning


CANCEL_BUTTON_NAMES = ["cancel_calculate_phase_table_button"]
REAL_PART = 1
IMAGINARY_PART = 2


class PhaseTableView(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(PhaseTableView, self).__init__(parent)
        self.phase_table_options_table = QtWidgets.QTableWidget(self)
        self.phasequad_table = QtWidgets.QTableWidget(self)
        self.setup_phase_table_options_table()
        self.setup_phasequad_table()
        self._old_backward_index = 1
        self._old_forward_index = 0
        self.setup_interface_layout()

        self.phasequad_table.cellChanged.connect(self.on_cell_changed)
        self.phasequad_table.itemChanged.connect(self.on_item_changed)
        self._on_table_data_changed = lambda: 0

        self.backward_group_combo.currentIndexChanged.connect(self.ensure_groups_different)
        self.forward_group_combo.currentIndexChanged.connect(self.ensure_groups_different)
        self.setEnabled(False)
        self._updating = False

    @property
    def first_good_time(self):
        return float(self.first_good_data_item.text())

    @first_good_time.setter
    def first_good_time(self, value):
        self.first_good_data_validator.last_valid_value = str(value)
        self.first_good_data_item.setText(str(value))

    @property
    def last_good_time(self):
        return float(self.last_good_data_item.text())

    @last_good_time.setter
    def last_good_time(self, value):
        self.last_good_data_validator.last_valid_value = str(value)
        self.last_good_data_item.setText(str(value))

    @property
    def input_workspace(self):
        return str(self.input_workspace_combo_box.currentText())

    @input_workspace.setter
    def input_workspace(self, value):
        index = self.input_workspace_combo_box.findText(value)
        if index != -1:
            self.input_workspace_combo_box.setCurrentIndex(index)

    @property
    def forward_group(self):
        return str(self.forward_group_combo.currentText())

    @forward_group.setter
    def forward_group(self, value):
        index = self.forward_group_combo.findText(value)
        if index != -1:
            self.forward_group_combo.setCurrentIndex(index)

    @property
    def backward_group(self):
        return str(self.backward_group_combo.currentText())

    @backward_group.setter
    def backward_group(self, value):
        index = self.backward_group_combo.findText(value)
        if index != -1:
            self.backward_group_combo.setCurrentIndex(index)

    @property
    def phase_table_for_phase_quad(self):
        return str(self.phase_table_selector_combo.currentText())

    @property
    def number_of_phase_tables(self):
        return self.phase_table_selector_combo.count()

    @phase_table_for_phase_quad.setter
    def phase_table_for_phase_quad(self, value):
        index = self.phase_table_selector_combo.findText(value)
        if index != -1:
            self.phase_table_selector_combo.setCurrentIndex(index)

    @property
    def output_fit_information(self):
        return self.output_fit_info_box.checkState() == QtCore.Qt.Checked

    def setup_interface_layout(self):
        # Properties table
        self.calculate_phase_table_button = QtWidgets.QToolButton()
        self.cancel_calculate_phase_table_button = QtWidgets.QToolButton()
        size_policy_2 = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Fixed)
        self.calculate_phase_table_button.setObjectName("calculate_phase_table_button")
        self.calculate_phase_table_button.setToolTip("")
        self.calculate_phase_table_button.setText("Calculate Phase Table")
        self.calculate_phase_table_button.setSizePolicy(size_policy_2)
        self.cancel_calculate_phase_table_button.setObjectName("cancel_calculate_phase_table_button")
        self.cancel_calculate_phase_table_button.setToolTip("")
        self.cancel_calculate_phase_table_button.setText("Cancel")
        self.cancel_calculate_phase_table_button.setSizePolicy(size_policy_2)

        # Phasetable selector
        self.phase_table_selector_label = QtWidgets.QLabel("Phase Table:")
        self.phase_table_selector_combo = QtWidgets.QComboBox()
        self.phase_table_selector_combo.setSizeAdjustPolicy(QtWidgets.QComboBox.AdjustToContents)
        self.phase_table_selector_spacer_item = QtWidgets.QSpacerItem(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)

        # Phasequad table
        self.add_phasequad_button = QtWidgets.QToolButton()
        self.remove_phasequad_button = QtWidgets.QToolButton()
        size_policy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(self.add_phasequad_button.sizePolicy().hasHeightForWidth())
        size_policy.setHeightForWidth(self.remove_phasequad_button.sizePolicy().hasHeightForWidth())

        self.add_phasequad_button.setSizePolicy(size_policy)
        self.add_phasequad_button.setObjectName("addPhaseQuadButton")
        self.add_phasequad_button.setToolTip("")
        self.add_phasequad_button.setText("+")
        self.remove_phasequad_button.setSizePolicy(size_policy)
        self.remove_phasequad_button.setObjectName("removePhaseQuadButton")
        self.remove_phasequad_button.setToolTip("")
        self.remove_phasequad_button.setText("-")
        self.phasequad_spacer_item = QtWidgets.QSpacerItem(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)

        # Layout
        self.phase_table_buttons_layout = QtWidgets.QHBoxLayout()
        self.phase_table_buttons_layout.setObjectName("phase_table_buttons_layout")
        self.phase_table_buttons_layout.addWidget(self.calculate_phase_table_button)
        self.phase_table_buttons_layout.addWidget(self.cancel_calculate_phase_table_button)

        self.phase_table_selector_layout = QtWidgets.QHBoxLayout()
        self.phase_table_selector_layout.setObjectName("phase_table_selector_layout")
        self.phase_table_selector_layout.addWidget(self.phase_table_selector_label)
        self.phase_table_selector_layout.addWidget(self.phase_table_selector_combo)
        self.phase_table_selector_layout.addItem(self.phase_table_selector_spacer_item)
        self.phase_table_selector_layout.setAlignment(QtCore.Qt.AlignLeft)

        self.phasequad_table_buttons_layout = QtWidgets.QHBoxLayout()
        self.phasequad_table_buttons_layout.setObjectName("phasequad_table_buttons_layout")
        self.phasequad_table_buttons_layout.addWidget(self.add_phasequad_button)
        self.phasequad_table_buttons_layout.addWidget(self.remove_phasequad_button)
        self.phasequad_table_buttons_layout.addItem(self.phasequad_spacer_item)
        self.phasequad_table_buttons_layout.setAlignment(QtCore.Qt.AlignLeft)

        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.vertical_layout.setObjectName("vertical_layout")
        self.vertical_layout.addWidget(self.phase_table_options_table)
        self.vertical_layout.addLayout(self.phase_table_buttons_layout)
        self.vertical_layout.addLayout(self.phase_table_selector_layout)
        self.vertical_layout.addWidget(self.phasequad_table)
        self.vertical_layout.addLayout(self.phasequad_table_buttons_layout)
        self.setLayout(self.vertical_layout)

    def set_input_combo_box(self, input_list):
        self.input_workspace_combo_box.clear()
        self.input_workspace_combo_box.addItems(input_list)
        self.input_workspace_combo_box.setCurrentIndex(0)

    def set_group_combo_boxes(self, group_list):
        self.forward_group_combo.clear()
        self.backward_group_combo.clear()

        self.forward_group_combo.addItems(group_list)
        self.backward_group_combo.addItems(group_list)

        self.forward_group_combo.setCurrentIndex(0)
        self.backward_group_combo.setCurrentIndex(1)

        self._old_backward_index = 1
        self._old_forward_index = 0

    def set_phase_table_combo_box(self, phase_table_list):
        self.phase_table_selector_combo.blockSignals(True)
        self.phase_table_selector_combo.clear()
        self.phase_table_selector_combo.addItems(phase_table_list)
        self.phase_table_selector_combo.setCurrentIndex(0)
        self.phase_table_selector_combo.blockSignals(False)

    def set_calculate_phase_table_action(self, action):
        self.calculate_phase_table_button.clicked.connect(action)

    def set_add_phasequad_action(self, action):
        self.add_phasequad_button.clicked.connect(action)

    def set_remove_phasequad_action(self, action):
        self.remove_phasequad_button.clicked.connect(action)

    def set_cancel_calculate_phase_table_action(self, action):
        self.cancel_calculate_phase_table_button.clicked.connect(action)

    def set_phase_table_changed_action(self, action):
        self.phase_table_selector_combo.currentTextChanged.connect(action)

    def ensure_groups_different(self):
        if self.backward_group_combo.currentText() == self.forward_group_combo.currentText():
            self.backward_group_combo.blockSignals(True)
            self.forward_group_combo.blockSignals(True)
            self.backward_group_combo.setCurrentIndex(self._old_forward_index)
            self.forward_group_combo.setCurrentIndex(self._old_backward_index)
            self.backward_group_combo.blockSignals(False)
            self.forward_group_combo.blockSignals(False)

        self._old_backward_index = self.backward_group_combo.currentIndex()
        self._old_forward_index = self.forward_group_combo.currentIndex()

    @staticmethod
    def warning_popup(message):
        warning(message)

    def enter_phasequad_name(self):
        new_pair_name, ok = QtWidgets.QInputDialog.getText(self, "Phasequad Name", "Enter name of new phasequad:")
        if ok:
            return new_pair_name

    def enter_phase_table_name(self):
        name, ok = QtWidgets.QInputDialog.getText(
            self, "Phase Table Name", "Enter the name of the new Phase table \n(leave blank to use the default name):"
        )
        if ok:
            return name

    def enable_widget(self):
        for widget in self.children():
            if str(widget.objectName()) in CANCEL_BUTTON_NAMES:
                continue
            widget.setEnabled(True)

    def disable_widget(self):
        for widget in self.children():
            if str(widget.objectName()) in CANCEL_BUTTON_NAMES:
                continue
            widget.setEnabled(False)

    def enable_updates(self):
        self._updating = False

    def disable_updates(self):
        self._updating = True

    def enable_phase_table_cancel(self):
        self.cancel_calculate_phase_table_button.setEnabled(True)

    def disable_phase_table_cancel(self):
        self.cancel_calculate_phase_table_button.setEnabled(False)

    def setup_phase_table_options_table(self):
        self.phase_table_options_table.setColumnCount(2)
        self.phase_table_options_table.setColumnWidth(0, 300)
        self.phase_table_options_table.setColumnWidth(1, 300)

        self.phase_table_options_table.setRowCount(6)
        self.phase_table_options_table.setHorizontalHeaderLabels(["Property", "Value"])
        header = self.phase_table_options_table.horizontalHeader()
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        vertical_headers = self.phase_table_options_table.verticalHeader()
        vertical_headers.setSectionsMovable(False)
        vertical_headers.setSectionResizeMode(QtWidgets.QHeaderView.ResizeToContents)
        vertical_headers.setVisible(True)
        self.phase_table_options_table.horizontalHeaderItem(0).setToolTip("")
        self.phase_table_options_table.horizontalHeaderItem(1).setToolTip("")

        # populate table
        options = []

        table_utils.setRowName(self.phase_table_options_table, 0, "Workspace")
        self.input_workspace_combo_box = table_utils.addComboToTable(self.phase_table_options_table, 0, options)

        table_utils.setRowName(self.phase_table_options_table, 1, "Forward group")
        self.forward_group_combo = table_utils.addComboToTable(self.phase_table_options_table, 1, options)

        table_utils.setRowName(self.phase_table_options_table, 2, "Backward group")
        self.backward_group_combo = table_utils.addComboToTable(self.phase_table_options_table, 2, options)

        table_utils.setRowName(self.phase_table_options_table, 3, "First Good Data")
        self.first_good_data_item, self.first_good_data_validator = table_utils.addDoubleToTable(
            self.phase_table_options_table, 0.1, 3, minimum=0.0
        )

        table_utils.setRowName(self.phase_table_options_table, 4, "Last Good Data")
        self.last_good_data_item, self.last_good_data_validator = table_utils.addDoubleToTable(
            self.phase_table_options_table, 15.0, 4, minimum=0.0
        )

        table_utils.setRowName(self.phase_table_options_table, 5, "Output fit information")
        self.output_fit_info_box = table_utils.addCheckBoxToTable(self.phase_table_options_table, False, 5)

    def setup_phasequad_table(self):
        self.phasequad_table.setColumnCount(3)
        self.phasequad_table.setHorizontalHeaderLabels(["Phasequad Name", "Analyse Re (plot/fit)", "Analyse Im (plot/fit)"])
        header = self.phasequad_table.horizontalHeader()
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(REAL_PART, QtWidgets.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(IMAGINARY_PART, QtWidgets.QHeaderView.ResizeToContents)
        vertical_headers = self.phasequad_table.verticalHeader()
        vertical_headers.setSectionsMovable(False)
        vertical_headers.setSectionResizeMode(QtWidgets.QHeaderView.ResizeToContents)
        vertical_headers.setVisible(True)
        self.phasequad_table.horizontalHeaderItem(0).setToolTip("")
        self.phasequad_table.horizontalHeaderItem(REAL_PART).setToolTip("")
        self.phasequad_table.horizontalHeaderItem(IMAGINARY_PART).setToolTip("")

    # Signal / Slot Connections
    def on_first_good_data_changed(self, slot):
        self.first_good_data_item.editingFinished.connect(slot)

    def on_last_good_data_changed(self, slot):
        self.last_good_data_item.editingFinished.connect(slot)

    # Phasequad Table Functionality
    def add_phasequad_to_table(self, name, to_analyse_re=True, to_analyse_im=True):
        row_position = self.phasequad_table.rowCount()
        self.phasequad_table.insertRow(row_position)
        name_item = QtWidgets.QTableWidgetItem(name)
        phasequad_name_widget = table_utils.ValidatedTableItem()
        phasequad_name_widget.setText(name)
        self.phasequad_table.setItem(row_position, 0, phasequad_name_widget)
        name_item.setFlags(QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable)
        self.phasequad_table.setItem(row_position, 0, name_item)

        for index, to_analyse in enumerate([to_analyse_re, to_analyse_im], start=1):
            analyse_item = QtWidgets.QTableWidgetItem(to_analyse)
            if to_analyse_re:
                analyse_item.setCheckState(QtCore.Qt.Checked)
            else:
                analyse_item.setCheckState(QtCore.Qt.Unchecked)
            self.phasequad_table.setItem(row_position, index, analyse_item)
            analyse_item.setFlags(QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled)
            self.phasequad_table.setItem(row_position, index, analyse_item)

    def on_phasequad_table_data_changed(self, slot):
        self._on_table_data_changed = slot

    def on_item_changed(self):
        """Not yet implemented."""
        pass

    def on_cell_changed(self, _row, _col):
        self._on_table_data_changed(_row, _col)

    def get_table_item(self, row, col):
        return self.phasequad_table.item(row, col)

    def get_table_item_text(self, row, col):
        return str(self.phasequad_table.item(row, col).text())

    def _get_selected_row_indices(self):
        return list(set(index.row() for index in self.phasequad_table.selectedIndexes()))

    def get_selected_phasequad_names_and_indexes(self):
        indexes = self._get_selected_row_indices()
        return [[str(self.phasequad_table.item(i, 0).text()), i] for i in indexes]

    def num_rows(self):
        return self.phasequad_table.rowCount()

    def num_cols(self):
        return self.phasequad_table.columnCount()

    def remove_last_row(self):
        last_row = self.phasequad_table.rowCount() - 1
        if last_row >= 0:
            self.phasequad_table.removeRow(last_row)

    def clear_phase_tables(self):
        self.phase_table_selector_combo.clear()

    def get_phase_table(self):
        return str(self.phase_table_selector_combo.currentText())

    def set_phase_table(self, table):
        self.phase_table_selector_combo.blockSignals(True)
        index = self.phase_table_selector_combo.findText(table)
        if index != -1:
            self.phase_table_selector_combo.setCurrentIndex(index)
        self.phase_table_selector_combo.blockSignals(False)

    def get_table_contents(self):
        if self._updating:
            return []
        return_table = [None for _ in range(self.num_rows())]
        for row in range(self.num_rows()):
            return_table[row] = str(self.phasequad_table.item(row, 0).text())
        return return_table

    def remove_phasequad_by_index(self, index):
        self.phasequad_table.removeRow(index)

    def clear_phasequads(self):
        for row in reversed(range(self.num_rows())):
            self.phasequad_table.removeRow(row)
