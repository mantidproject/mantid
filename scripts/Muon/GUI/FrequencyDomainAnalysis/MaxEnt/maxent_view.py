# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui

from Muon.GUI.Common.utilities import table_utils


construct = "Construct"


class MaxEntView(QtGui.QWidget):

    """
    The view for the MaxEnt widget. This
    creates the look of the widget
    """
    # signals
    maxEntButtonSignal = QtCore.pyqtSignal()
    cancelSignal = QtCore.pyqtSignal()
    phaseSignal = QtCore.pyqtSignal(object, object)

    def __init__(self, parent=None):
        super(MaxEntView, self).__init__(parent)
        self.grid = QtGui.QVBoxLayout(self)

        # add splitter for resizing
        splitter = QtGui.QSplitter(QtCore.Qt.Vertical)

        self.run = None
        # make table
        self.table = QtGui.QTableWidget(self)
        self.table.resize(800, 800)

        self.table.setRowCount(11)
        self.table.setColumnCount(2)
        self.table.setColumnWidth(0, 300)
        self.table.setColumnWidth(1, 300)
        self.table.verticalHeader().setVisible(False)
        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.setHorizontalHeaderLabels(
            ("MaxEnt Property;Value").split(";"))
        table_utils.setTableHeaders(self.table)

        # populate table
        options = ['test']

        table_utils.setRowName(self.table, 0, "Workspace")
        self.ws = table_utils.addComboToTable(self.table, 0, options)

        table_utils.setRowName(self.table, 1, "First good time")
        self.first_good = table_utils.addDoubleToTable(self.table, 0.1, 1)

        table_utils.setRowName(self.table, 2, "Last good time")
        self.last_good = table_utils.addDoubleToTable(self.table, 15.0, 2)

        table_utils.setRowName(self.table, 3, "Fit dead times")
        self.dead_box = table_utils.addCheckBoxToTable(self.table, True, 3)

        table_utils.setRowName(self.table, 4, "Use Phase Table")
        self.use_phaseTable_box = table_utils.addCheckBoxToTable(
            self.table, False, 4)

        table_utils.setRowName(self.table, 5, "Select Phase Table")
        options = [construct]
        self.phaseTable_box = table_utils.addComboToTable(
            self.table, 5, options)

        table_utils.setRowName(self.table, 6, "Fix phases")
        self.fix_phase_box = table_utils.addCheckBoxToTable(
            self.table, False, 6)

        self.table.hideRow(5)
        self.table.hideRow(6)

        table_utils.setRowName(self.table, 7, "Output phase table")
        self.output_phase_box = table_utils.addCheckBoxToTable(
            self.table, False, 7)

        table_utils.setRowName(self.table, 8, "Output deadtimes")
        self.output_dead_box = table_utils.addCheckBoxToTable(
            self.table, False, 8)

        table_utils.setRowName(self.table, 9, "Output reconstructed data")
        self.output_data_box = table_utils.addCheckBoxToTable(
            self.table, False, 9)

        table_utils.setRowName(self.table, 10, "Output phase convergence")
        self.output_phase_evo_box = table_utils.addCheckBoxToTable(
            self.table, False, 10)

        self.table.resizeRowsToContents()

        # advanced options table
        self.advancedLabel = QtGui.QLabel("\n  Advanced Options")
        # make table
        self.tableA = QtGui.QTableWidget(self)
        self.tableA.resize(800, 800)

        self.tableA.setRowCount(7)
        self.tableA.setColumnCount(2)
        self.tableA.setColumnWidth(0, 300)
        self.tableA.setColumnWidth(1, 300)

        self.tableA.verticalHeader().setVisible(False)
        self.tableA.horizontalHeader().setStretchLastSection(True)

        self.tableA.setHorizontalHeaderLabels(
            ("Advanced Property;Value").split(";"))
        table_utils.setTableHeaders(self.tableA)

        table_utils.setRowName(self.tableA, 0, "Maximum entropy constant (A)")
        self.AConst = table_utils.addDoubleToTable(self.tableA, 0.1, 0)

        table_utils.setRowName(self.tableA, 1, "Lagrange multiplier for chi^2")
        self.factor = table_utils.addDoubleToTable(self.tableA, 1.04, 1)

        table_utils.setRowName(self.tableA, 2, "Inner Iterations")
        self.inner_loop = table_utils.addSpinBoxToTable(self.tableA, 10, 2)

        table_utils.setRowName(self.tableA, 3, "Outer Iterations")
        self.outer_loop = table_utils.addSpinBoxToTable(self.tableA, 10, 3)

        table_utils.setRowName(self.tableA, 4, "Double pulse data")
        self.double_pulse_box = table_utils.addCheckBoxToTable(
            self.tableA, False, 4)

        table_utils.setRowName(self.tableA, 5, "Number of data points")
        self.N_points = table_utils.addComboToTable(self.tableA, 5, options)

        table_utils.setRowName(self.tableA, 6, "Maximum Field ")
        self.max_field = table_utils.addDoubleToTable(self.tableA, 1000.0, 6)

        # layout
        # this is if complex data is unhidden
        self.table.setMinimumSize(40, 203)
        self.tableA.setMinimumSize(40, 207)

        # make buttons
        self.button = QtGui.QPushButton('Calculate MaxEnt', self)
        self.button.setStyleSheet("background-color:lightgrey")
        self.cancel = QtGui.QPushButton('Cancel', self)
        self.cancel.setStyleSheet("background-color:lightgrey")
        self.cancel.setEnabled(False)
        # connects
        self.button.clicked.connect(self.MaxEntButtonClick)
        self.cancel.clicked.connect(self.cancelClick)
        self.table.cellClicked.connect(self.phaseBoxClick)
        # button layout
        self.buttonLayout = QtGui.QHBoxLayout()
        self.buttonLayout.addWidget(self.button)
        self.buttonLayout.addWidget(self.cancel)
        # add to layout
        splitter.addWidget(self.table)
        splitter.addWidget(self.advancedLabel)
        splitter.addWidget(self.tableA)
        self.grid.addWidget(splitter)
        self.grid.addLayout(self.buttonLayout)

    def getLayout(self):
        return self.grid

    # add data to view
    def addItems(self, options):
        self.ws.clear()
        self.ws.addItems(options)

    def clearPhaseTables(self):
        self.phaseTable_box.clear()
        self.phaseTable_box.addItems([construct])

    def getPhaseTableIndex(self):
        return self.phaseTable_box.currentIndex()

    def setPhaseTableIndex(self, index):
        self.phaseTable_box.setCurrentIndex(index)

    def getPhaseTableOptions(self):
        return [self.phaseTable_box.itemText(j) for j in range(self.phaseTable_box.count())]

    def addPhaseTableToGUI(self, option):
        self.phaseTable_box.addItem(option)

    def addNPoints(self, options):
        self.N_points.clear()
        self.N_points.addItems(options)

    def setRun(self, run):
        self.run = run

    def changedPhaseBox(self):
        self.table.setRowHidden(
            5,
            self.use_phaseTable_box.checkState(
            ) != QtCore.Qt.Checked)
        self.table.setRowHidden(
            6,
            self.use_phaseTable_box.checkState(
            ) != QtCore.Qt.Checked)

    # send signal
    def MaxEntButtonClick(self):
        self.maxEntButtonSignal.emit()

    def cancelClick(self):
        self.cancelSignal.emit()

    def phaseBoxClick(self, row, col):
        self.phaseSignal.emit(row, col)

    # get some inputs for model
    def initMaxEntInput(self):
        inputs = {}

        #  this will be removed once maxEnt does a simultaneous fit
        inputs['InputWorkspace'] = str(self.ws.currentText())
        # will use this instead of the above
        inputs["FirstGoodTime"] = float(self.first_good.text())
        inputs['LastGoodTime'] = float(self.last_good.text())

        inputs["Npts"] = int(self.N_points.currentText())
        inputs["MaxField"] = float(self.max_field.text())
        inputs["FitDeadTime"] = self.dead_box.checkState() == QtCore.Qt.Checked
        inputs["DoublePulse"] = self.double_pulse_box.checkState() == QtCore.Qt.Checked
        inputs["OuterIterations"] = int(self.outer_loop.text())
        inputs["InnerIterations"] = int(self.inner_loop.text())
        inputs["DefaultLevel"] = float(self.AConst.text())
        inputs["Factor"] = float(self.factor.text())

        # will remove this when sim maxent Works
        out = self.run.replace(";", "; ")
        inputs['OutputWorkspace'] = out + ";" + \
            str(self.ws.currentText()) + ";FrequencyDomain;MaxEnt"
        return inputs

    def addPhaseTable(self, inputs):
        inputs["FixPhases"] = self.fix_phase_box.checkState() == QtCore.Qt.Checked
        if self.usePhases() and self.phaseTable_box.currentText() == construct:
            inputs['InputPhaseTable'] = "PhaseTable"
        elif self.usePhases():
            inputs['InputPhaseTable'] = self.phaseTable_box.currentText()

    def outputPhases(self):
        return self.output_phase_box.checkState() == QtCore.Qt.Checked

    def addOutputPhases(self, inputs):
        inputs['OutputPhaseTable'] = self.run + ";" + \
            str(self.ws.currentText()) + ";PhaseTable;MaxEnt"

    def outputDeadTime(self):
        return self.output_dead_box.checkState() == QtCore.Qt.Checked

    def addOutputDeadTime(self, inputs):
        inputs['OutputDeadTimeTable'] = self.run + ";" + \
            str(self.ws.currentText()) + ";DeadTimeTable;MaxEnt"

    def outputPhaseEvo(self):
        return self.output_phase_evo_box.checkState() == QtCore.Qt.Checked

    def addOutputPhaseEvo(self, inputs):
        inputs['PhaseConvergenceTable'] = self.run + ";" + \
            str(self.ws.currentText()) + ";PhaseConvergenceTable;MaxEnt"

    def outputTime(self):
        return self.output_data_box.checkState() == QtCore.Qt.Checked

    def addOutputTime(self, inputs):
        inputs['ReconstructedSpectra'] = self.run + ";" + \
            str(self.ws.currentText()) + ";TimeDomain;MaxEnt"

    def calcPhases(self):
        return self.phaseTable_box.currentText() == construct

    def usePhases(self):
        return self.use_phaseTable_box.checkState() == QtCore.Qt.Checked

    def getInputWS(self):
        return str(self.ws.currentText())

#    def calcPhasesInit(self):
#        inputs={}
#
# this will be removed once maxEnt does a simultaneous fit
#        inputs['InputWorkspace']=str( self.ws.currentText())
# will use this instead of the above
#        inputs["FirstGoodData"]= float( self.first_good.text())
#        inputs['LastGoodData']=float(self.last_good.text())
#        inputs["DetectorTable"] = "PhaseTable"
#        inputs["DataFitted"] = "fits"
#
#        return inputs
    def getFirstGoodData(self):
        return float(self.first_good.text())

    def getLastGoodData(self):
        return float(self.last_good.text())
    # turn button on and off

    def activateCalculateButton(self):
        self.button.setEnabled(True)
        self.cancel.setEnabled(False)

    def deactivateCalculateButton(self):
        self.button.setEnabled(False)
        self.cancel.setEnabled(True)
