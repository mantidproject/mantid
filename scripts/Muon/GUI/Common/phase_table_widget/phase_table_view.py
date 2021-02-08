# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore
from Muon.GUI.Common.utilities import table_utils
from Muon.GUI.Common.message_box import warning
from mantidqt.utils.qt import load_ui

ui_muon_phases_tab, _ = load_ui(__file__, "muon_phases_tab.ui")


class PhaseTableView(QtWidgets.QWidget, ui_muon_phases_tab):
    def __init__(self, parent=None):
        super(PhaseTableView, self).__init__(parent)
        self.setupUi(self)
        self._old_backward_index = 1
        self._old_forward_index = 0
        self.setup_phase_table_options_table()

        self.backward_group_combo.currentIndexChanged.connect(self.ensure_groups_different)
        self.forward_group_combo.currentIndexChanged.connect(self.ensure_groups_different)
        self.setEnabled(False)

    @property
    def first_good_time(self):
        return float(self.first_good_data_item.text())

    @first_good_time.setter
    def first_good_time(self, value):
        self.first_good_data_item.setText(str(value))

    @property
    def last_good_time(self):
        return float(self.last_good_data_item.text())

    @last_good_time.setter
    def last_good_time(self, value):
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
        return str(self.phase_quad_phase_table_combo.currentText())

    @property
    def number_of_phase_tables(self):
        return self.phase_quad_phase_table_combo.count()

    @phase_table_for_phase_quad.setter
    def phase_table_for_phase_quad(self, value):
        index = self.phase_quad_phase_table_combo.findText(value)
        if index != -1:
            self.phase_quad_phase_table_combo.setCurrentIndex(index)

    @property
    def output_fit_information(self):
        return self.output_fit_info_box.checkState() == QtCore.Qt.Checked

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
        self.phase_quad_phase_table_combo.clear()

        self.phase_quad_phase_table_combo.addItems(phase_table_list)

        self.phase_quad_phase_table_combo.setCurrentIndex(0)

    def set_calculate_phase_table_action(self, action):
        self.calculate_phase_table_button.clicked.connect(action)

    def set_calculate_phase_quad_action(self, action):
        self.calculate_phase_quad_button.clicked.connect(action)

    def set_cancel_action(self, action):
        self.cancel_button.clicked.connect(action)
        self.phasequad_cancel_button.clicked.connect(action)

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

    def enter_pair_name(self):
        new_pair_name, ok = QtWidgets.QInputDialog.getText(self, 'Phasequad Name', 'Enter name of new phasequad:')
        if ok:
            return new_pair_name

    def enable_widget(self):
        for widget in self.children():
            if str(widget.objectName()) in ['cancel_button', 'phasequad_cancel_button']:
                continue
            widget.setEnabled(True)

    def disable_widget(self):
        for widget in self.children():
            if str(widget.objectName()) == ['cancel_button', 'phasequad_cancel_button']:
                continue
            widget.setEnabled(False)

    def enable_cancel(self):
        self.cancel_button.setEnabled(True)

    def enable_phasequad_cancel(self):
        self.phasequad_cancel_button.setEnabled(True)

    def disable_cancel(self):
        self.cancel_button.setEnabled(False)
        self.phasequad_cancel_button.setEnabled(False)

    def setup_phase_table_options_table(self):
        self.phase_table_options_table.setColumnWidth(0, 300)
        self.phase_table_options_table.setColumnWidth(1, 300)

        # populate table
        options = []

        table_utils.setRowName(self.phase_table_options_table, 0, "Workspace")
        self.input_workspace_combo_box = table_utils.addComboToTable(self.phase_table_options_table, 0, options)

        table_utils.setRowName(self.phase_table_options_table, 1, "Forward group")
        self.forward_group_combo = table_utils.addComboToTable(self.phase_table_options_table, 1, options)

        table_utils.setRowName(self.phase_table_options_table, 2, "Backward group")
        self.backward_group_combo = table_utils.addComboToTable(self.phase_table_options_table, 2, options)

        table_utils.setRowName(self.phase_table_options_table, 3, "First Good Data")
        self.first_good_data_item = table_utils.addDoubleToTable(self.phase_table_options_table, 0.1, 3)

        table_utils.setRowName(self.phase_table_options_table, 4, "Last Good Data")
        self.last_good_data_item = table_utils.addDoubleToTable(self.phase_table_options_table, 15.0, 4)

        table_utils.setRowName(self.phase_table_options_table, 5, "Output fit information")
        self.output_fit_info_box = table_utils.addCheckBoxToTable(
            self.phase_table_options_table, False, 5)

        self.phase_table_options_table.resizeRowsToContents()

        table_utils.setTableHeaders(self.phase_table_options_table)

        self.phase_quad_table.setColumnWidth(0, 300)

        table_utils.setRowName(self.phase_quad_table, 0, 'PhaseTable')
        self.phase_quad_phase_table_combo = table_utils.addComboToTable(self.phase_quad_table, 0, options)

        self.phase_quad_table.resizeRowsToContents()

        # add to layout
        table_utils.setTableHeaders(self.phase_quad_table)

    # Signal / Slot Connections
    def on_first_good_data_changed(self, slot):
        self.first_good_data_item.editingFinished.connect(slot)

    def on_last_good_data_changed(self, slot):
        self.last_good_data_item.editingFinished.connect(slot)
