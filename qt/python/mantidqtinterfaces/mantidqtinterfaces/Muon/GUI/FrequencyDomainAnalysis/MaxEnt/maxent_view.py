# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore

from mantidqtinterfaces.Muon.GUI.Common.utilities import table_utils
from mantidqtinterfaces.Muon.GUI.Common.message_box import warning
from mantidqtinterfaces.Muon.GUI.Common.data_selectors.cyclic_data_selector_view import CyclicDataSelectorView


class MaxEntView(QtWidgets.QWidget):
    """
    The view for the MaxEnt widget. This
    creates the look of the widget
    """

    # signals
    maxEntButtonSignal = QtCore.Signal()
    cancelSignal = QtCore.Signal()

    def __init__(self, parent=None):
        super(MaxEntView, self).__init__(parent)
        self.grid = QtWidgets.QVBoxLayout(self)

        self._runs_selector = CyclicDataSelectorView(self)
        self._runs_selector.set_data_combo_box_label("Runs:")
        self._runs_selector.set_data_combo_box_label_width(50)

        self._period_selector = CyclicDataSelectorView(self)
        self._period_selector.set_data_combo_box_label("Period:")
        self._period_selector.set_data_combo_box_label_width(50)

        # add splitter for resizing
        splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical)

        self.run = None
        # make table
        self.table = QtWidgets.QTableWidget(self)
        self.table.resize(800, 800)

        self.table.setRowCount(7)
        self.table.setColumnCount(2)
        self.table.setColumnWidth(0, 300)
        self.table.setColumnWidth(1, 300)
        self.table.verticalHeader().setVisible(False)
        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.setHorizontalHeaderLabels(("MaxEnt Property;Value").split(";"))
        table_utils.setTableHeaders(self.table)

        # populate table
        options = []

        table_utils.setRowName(self.table, 0, "Calculate by")
        self.method = table_utils.addComboToTable(self.table, 0, options)

        table_utils.setRowName(self.table, 1, "Phase Table")
        self.phase_table_combo = table_utils.addComboToTable(self.table, 1, options)

        table_utils.setRowName(self.table, 2, "Fit dead times")
        self.dead_box = table_utils.addCheckBoxToTable(self.table, True, 2)

        table_utils.setRowName(self.table, 3, "Output phase table")
        self.output_phase_box = table_utils.addCheckBoxToTable(self.table, False, 3)

        table_utils.setRowName(self.table, 4, "Output deadtimes")
        self.output_dead_box = table_utils.addCheckBoxToTable(self.table, False, 4)

        table_utils.setRowName(self.table, 5, "Output reconstructed data")
        self.output_data_box = table_utils.addCheckBoxToTable(self.table, False, 5)

        table_utils.setRowName(self.table, 6, "Output phase convergence")
        self.output_phase_evo_box = table_utils.addCheckBoxToTable(self.table, False, 6)

        self.table.resizeRowsToContents()

        # advanced options table
        self.advancedLabel = QtWidgets.QLabel("\n  Advanced Options")
        # make table
        self.tableA = QtWidgets.QTableWidget(self)
        self.tableA.resize(800, 800)

        self.tableA.setRowCount(7)
        self.tableA.setColumnCount(2)
        self.tableA.setColumnWidth(0, 300)
        self.tableA.setColumnWidth(1, 300)

        self.tableA.verticalHeader().setVisible(False)
        self.tableA.horizontalHeader().setStretchLastSection(True)

        self.tableA.setHorizontalHeaderLabels(("Advanced Property;Value").split(";"))
        table_utils.setTableHeaders(self.tableA)

        table_utils.setRowName(self.tableA, 0, "Maximum entropy constant (A)")
        self.AConst, _ = table_utils.addDoubleToTable(self.tableA, 0.1, 0, minimum=0.0)

        table_utils.setRowName(self.tableA, 1, "Lagrange multiplier for chi^2")
        self.factor, _ = table_utils.addDoubleToTable(self.tableA, 1.04, 1, minimum=0.0)

        table_utils.setRowName(self.tableA, 2, "Inner Iterations")
        self.inner_loop = table_utils.addSpinBoxToTable(self.tableA, 10, 2)

        table_utils.setRowName(self.tableA, 3, "Outer Iterations")
        self.outer_loop = table_utils.addSpinBoxToTable(self.tableA, 10, 3)

        table_utils.setRowName(self.tableA, 4, "Double pulse data")
        self.double_pulse_box = table_utils.addCheckBoxToTable(self.tableA, False, 4)

        table_utils.setRowName(self.tableA, 5, "Number of data points")
        self.N_points = table_utils.addComboToTable(self.tableA, 5, options)

        table_utils.setRowName(self.tableA, 6, "Maximum Field ")
        self.max_field, _ = table_utils.addDoubleToTable(self.tableA, 1000.0, 6, minimum=0.0)

        # layout
        # this is if complex data is unhidden
        self.table.setMinimumSize(40, 203)
        self.tableA.setMinimumSize(40, 207)

        # make buttons
        self.button = QtWidgets.QPushButton("Calculate MaxEnt", self)
        self.button.setStyleSheet("background-color:lightgrey")
        self.cancel = QtWidgets.QPushButton("Cancel", self)
        self.cancel.setStyleSheet("background-color:lightgrey")
        self.cancel.setEnabled(False)
        # connects
        self.button.clicked.connect(self.MaxEntButtonClick)
        self.cancel.clicked.connect(self.cancelClick)
        # button layout
        self.buttonLayout = QtWidgets.QHBoxLayout()
        self.buttonLayout.addWidget(self.button)
        self.buttonLayout.addWidget(self.cancel)
        # add to layout
        self.grid.addWidget(self._runs_selector)
        self.grid.addWidget(self._period_selector)
        splitter.addWidget(self.table)
        splitter.addWidget(self.advancedLabel)
        splitter.addWidget(self.tableA)
        self.grid.addWidget(splitter)
        self.grid.addLayout(self.buttonLayout)

    def getLayout(self):
        return self.grid

    # add data to view
    def addRuns(self, runs):
        self._runs_selector.update_dataset_name_combo_box(runs)

    def add_periods(self, periods):
        self._period_selector.update_dataset_name_combo_box(periods)

    def set_methods(self, options):
        self.method.clear()
        self.method.addItems(options)

    def addNPoints(self, options):
        self.N_points.clear()
        self.N_points.addItems(options)

    # send signal
    def MaxEntButtonClick(self):
        self.maxEntButtonSignal.emit()

    def cancelClick(self):
        self.cancelSignal.emit()

    def warning_popup(self, message):
        warning(message, parent=self)

    def activateCalculateButton(self):
        self.button.setEnabled(True)
        self._period_selector.setEnabled(True)
        self._runs_selector.setEnabled(True)
        self.cancel.setEnabled(False)

    def deactivateCalculateButton(self):
        self.button.setEnabled(False)
        self._period_selector.setEnabled(False)
        self._runs_selector.setEnabled(False)
        self.cancel.setEnabled(True)

    def update_phase_table_combo(self, phase_table_list):
        name = self.phase_table_combo.currentText()

        self.phase_table_combo.clear()
        self.phase_table_combo.addItems(phase_table_list)

        index = self.phase_table_combo.findText(name)

        if index != -1:
            self.phase_table_combo.setCurrentIndex(index)
        else:
            self.phase_table_combo.setCurrentIndex(0)

    # slots
    def run_changed_slot(self, slot):
        self._runs_selector.set_slot_for_dataset_changed(slot)

    def method_changed_slot(self, slot):
        self.method.currentIndexChanged.connect(slot)

    def period_changed_slot(self, slot):
        self._period_selector.set_slot_for_dataset_changed(slot)

    @property
    def get_run(self):
        return str(self._runs_selector.current_dataset_name)

    @property
    def num_periods(self):
        return len(self._period_selector.dataset_names)

    @property
    def get_period(self):
        return str(self._period_selector.current_dataset_name)

    @property
    def get_method(self):
        return str(self.method.currentText())

    @property
    def num_points(self):
        return int(self.N_points.currentText())

    @property
    def maximum_field(self):
        return float(self.max_field.text())

    @property
    def fit_dead_times(self):
        return self.dead_box.checkState() == QtCore.Qt.Checked

    @property
    def double_pulse(self):
        return self.double_pulse_box.checkState() == QtCore.Qt.Checked

    @property
    def outer_iterations(self):
        return int(self.outer_loop.text())

    @property
    def inner_iterations(self):
        return int(self.inner_loop.text())

    @property
    def maximum_entropy_constant(self):
        return float(self.AConst.text())

    @property
    def lagrange_multiplier(self):
        return float(self.factor.text())

    @property
    def phase_table(self):
        return str(self.phase_table_combo.currentText())

    @property
    def output_phase_table(self):
        return self.output_phase_box.checkState() == QtCore.Qt.Checked

    @property
    def output_dead_times(self):
        return self.output_dead_box.checkState() == QtCore.Qt.Checked

    @property
    def output_phase_convergence(self):
        return self.output_phase_evo_box.checkState() == QtCore.Qt.Checked

    @property
    def output_reconstructed_spectra(self):
        return self.output_data_box.checkState() == QtCore.Qt.Checked
