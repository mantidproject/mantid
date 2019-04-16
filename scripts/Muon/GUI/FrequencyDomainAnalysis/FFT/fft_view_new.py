# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets, QtCore

import mantid.simpleapi as mantid

from Muon.GUI.Common.utilities import table_utils
from Muon.GUI.Common.message_box import warning


class FFTView(QtWidgets.QWidget):

    """
    creates the layout for the FFT GUI
    """
    # signals
    buttonSignal = QtCore.Signal()
    tableClickSignal = QtCore.Signal(object, object)
    phaseCheckSignal = QtCore.Signal()

    def __init__(self, parent=None):
        super(FFTView, self).__init__(parent)
        self.grid = QtWidgets.QGridLayout(self)

        # add splitter for resizing
        splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical)

        # make table
        self.FFTTable = QtWidgets.QTableWidget(self)
        self.FFTTable.resize(800, 800)
        self.FFTTable.setRowCount(6)
        self.FFTTable.setColumnCount(2)
        self.FFTTable.setColumnWidth(0, 300)
        self.FFTTable.setColumnWidth(1, 300)
        self.FFTTable.verticalHeader().setVisible(False)
        self.FFTTable.horizontalHeader().setStretchLastSection(True)
        self.FFTTable.setHorizontalHeaderLabels(
            ("FFT Property;Value").split(";"))
        # populate table
        options = ['test']

        table_utils.setRowName(self.FFTTable, 0, "Workspace")
        self.ws = table_utils.addComboToTable(self.FFTTable, 0, options)
        self.Im_box_row = 1
        table_utils.setRowName(
            self.FFTTable,
            self.Im_box_row,
            "Imaginary Data")
        self.Im_box = table_utils.addCheckBoxToTable(
            self.FFTTable, True, self.Im_box_row)

        table_utils.setRowName(self.FFTTable, 2, "Imaginary Workspace")
        self.Im_ws = table_utils.addComboToTable(self.FFTTable, 2, options)

        self.shift_box_row = 3
        table_utils.setRowName(self.FFTTable, self.shift_box_row, "Auto shift")
        self.shift_box = table_utils.addCheckBoxToTable(
            self.FFTTable, True, self.shift_box_row)

        table_utils.setRowName(self.FFTTable, 4, "Shift")
        self.shift = table_utils.addDoubleToTable(self.FFTTable, 0.0, 4)
        self.FFTTable.hideRow(4)

        table_utils.setRowName(self.FFTTable, 5, "Use Raw data")
        self.Raw_box = table_utils.addCheckBoxToTable(self.FFTTable, True, 5)

        self.FFTTable.resizeRowsToContents()
        # make advanced table options
        self.advancedLabel = QtWidgets.QLabel("\n Advanced Options")
        self.FFTTableA = QtWidgets.QTableWidget(self)
        self.FFTTableA.resize(800, 800)
        self.FFTTableA.setRowCount(4)
        self.FFTTableA.setColumnCount(2)
        self.FFTTableA.setColumnWidth(0, 300)
        self.FFTTableA.setColumnWidth(1, 300)
        self.FFTTableA.verticalHeader().setVisible(False)
        self.FFTTableA.horizontalHeader().setStretchLastSection(True)
        self.FFTTableA.setHorizontalHeaderLabels(
            ("Advanced Property;Value").split(";"))

        table_utils.setRowName(self.FFTTableA, 0, "Apodization Function")
        options = ["Lorentz", "Gaussian", "None"]
        self.apodization = table_utils.addComboToTable(
            self.FFTTableA, 0, options)

        table_utils.setRowName(
            self.FFTTableA,
            1,
            "Decay Constant (micro seconds)")
        self.decay = table_utils.addDoubleToTable(self.FFTTableA, 4.4, 1)

        table_utils.setRowName(self.FFTTableA, 2, "Negative Padding")
        self.negativePadding = table_utils.addCheckBoxToTable(
            self.FFTTableA, True, 2)

        table_utils.setRowName(self.FFTTableA, 3, "Padding")
        self.padding = table_utils.addSpinBoxToTable(self.FFTTableA, 1, 3)
        self.FFTTableA.resizeRowsToContents()

        # make button
        self.button = QtWidgets.QPushButton('Calculate FFT', self)
        self.button.setStyleSheet("background-color:lightgrey")
        # connects
        self.FFTTable.cellClicked.connect(self.tableClick)
        self.button.clicked.connect(self.buttonClick)
        self.ws.currentIndexChanged.connect(self.phaseCheck)
        # add to layout
        self.FFTTable.setMinimumSize(40, 158)
        self.FFTTableA.setMinimumSize(40, 127)
        table_utils.setTableHeaders(self.FFTTable)
        table_utils.setTableHeaders(self.FFTTableA)

        # add to layout
        splitter.addWidget(self.FFTTable)
        splitter.addWidget(self.advancedLabel)
        splitter.addWidget(self.FFTTableA)
        self.grid.addWidget(splitter)
        self.grid.addWidget(self.button)

    def getLayout(self):
        return self.grid

  # add data to view
    def addItems(self, options):
        self.ws.clear()
        self.ws.addItems(options)
        self.Im_ws.clear()
        self.Im_ws.addItems(options)
        self.phaseQuadChanged()

    def removeIm(self, pattern):
        index = self.Im_ws.findText(pattern)
        self.Im_ws.removeItem(index)

    def removeRe(self, pattern):
        index = self.ws.findText(pattern)
        self.ws.removeItem(index)

    def setReTo(self, name):
        index = self.ws.findText(name)
        if index == -1:
            return
        self.ws.setCurrentIndex(index)

    def setImTo(self, name):
        index = self.Im_ws.findText(name)
        if index == -1:
            return
        self.Im_ws.setCurrentIndex(index)

    # connect signals
    def phaseCheck(self):
        self.phaseCheckSignal.emit()

    def tableClick(self, row, col):
        self.tableClickSignal.emit(row, col)

    def buttonClick(self):
        self.buttonSignal.emit()

    def getInputWS(self):
        return self.ws.currentText()

    def getInputImWS(self):
        return self.Im_ws.currentText()

    # responses to commands
    def activateButton(self):
        self.button.setEnabled(True)

    def deactivateButton(self):
        self.button.setEnabled(False)

    def setPhaseBox(self):
        self.FFTTable.setRowHidden(8, "PhaseQuad" not in self.getWS())

    def changed(self, box, row):
        self.FFTTable.setRowHidden(row, box.checkState() == QtCore.Qt.Checked)

    def changedHideUnTick(self, box, row):
        self.FFTTable.setRowHidden(row, box.checkState() != QtCore.Qt.Checked)

    def phaseQuadChanged(self):
        # hide complex ws
        self.FFTTable.setRowHidden(2, "PhaseQuad" in self.getWS())

    def initFFTInput(self, run=None):
        inputs = {}
        inputs[
            'InputWorkspace'] = "__ReTmp__"  #
        inputs['Real'] = 0  # always zero
        out = str(self.ws.currentText()).replace(";", "; ")
        if run is None:
            run = self.getRunName()
        inputs['OutputWorkspace'] = run + ";" + out + ";FFT"
        inputs["AcceptXRoundingErrors"] = True
        return inputs

    def addFFTComplex(self, inputs):
        inputs["InputImagWorkspace"] = "__ImTmp__"
        inputs["Imaginary"] = 0  # always zero

    def addFFTShift(self, inputs):
        inputs['AutoShift'] = False
        inputs['Shift'] = float(self.shift.text())

    def addRaw(self, inputs, key):
        inputs[key] += "_Raw"

    def getFFTRePhase(self, inputs):
        inputs['InputWorkspace'] = "__ReTmp__"
        inputs['Real'] = 0  # always zero

    def getFFTImPhase(self, inputs):
        inputs['InputImagWorkspace'] = "__ReTmp__"
        inputs['Imaginary'] = 1

    def initAdvanced(self):
        inputs = {}
        inputs["ApodizationFunction"] = str(self.apodization.currentText())
        inputs["DecayConstant"] = float(self.decay.text())
        inputs["NegativePadding"] = self.negativePadding.checkState()
        inputs["Padding"] = int(self.padding.text())
        return inputs

    def ReAdvanced(self, inputs):
        inputs['InputWorkspace'] = str(
            self.ws.currentText()).replace(";",
                                           "; ")
        inputs['OutputWorkspace'] = "__ReTmp__"

    def ImAdvanced(self, inputs):
        inputs['InputWorkspace'] = str(
            self.Im_ws.currentText()).replace(";",
                                              "; ")
        inputs['OutputWorkspace'] = "__ImTmp__"

    def RePhaseAdvanced(self, inputs):
        inputs['InputWorkspace'] = "__phaseQuad__"
        inputs['OutputWorkspace'] = "__ReTmp__"

    # get methods (from the GUI)
    def getWS(self):
        return str(self.ws.currentText()).replace(";", "; ")

    def isAutoShift(self):
        return self.shift_box.checkState() == QtCore.Qt.Checked

    def isComplex(self):
        return self.Im_box.checkState() == QtCore.Qt.Checked

    def isRaw(self):
        return self.Raw_box.checkState() == QtCore.Qt.Checked

    def set_raw_checkbox_state(self, state):
        if state:
            self.Raw_box.setCheckState(QtCore.Qt.Checked)
        else:
            self.Raw_box.setCheckState(QtCore.Qt.Unchecked)

    def setup_raw_checkbox_changed(self, slot):
        self.FFTTable.itemChanged.connect(self.raw_checkbox_changed)
        self.signal_raw_option_changed = slot

    def raw_checkbox_changed(self, table_item):
        if table_item == self.Raw_box:
            self.signal_raw_option_changed()

    def getImBoxRow(self):
        return self.Im_box_row

    def getShiftBoxRow(self):
        return self.shift_box_row

    def getImBox(self):
        return self.Im_box

    def getShiftBox(self):
        return self.shift_box

    def getFirstGoodData(self):
        return float(self.x0.text())

    def getLastGoodData(self):
        return (self.xN.text())

    def isNewPhaseTable(self):
        return self.phaseTable_box.checkState() == QtCore.Qt.Checked

    def isPhaseBoxShown(self):
        return self.FFTTable.isRowHidden(8)

    def warning_popup(self, message):
        warning(message, parent=self)

    @property
    def workspace(self):
        return str(self.ws.currentText())

    @property
    def imaginary_workspace(self):
        return str(self.Im_ws.currentText())

    @property
    def imaginary_data(self):
        return self.Im_box.checkState() == QtCore.Qt.Checked

    @property
    def auto_shift(self):
        return self.shift_box.checkState() == QtCore.Qt.Checked

    @property
    def use_raw_data(self):
        return self.Raw_box.checkState() == QtCore.Qt.Checked

    @property
    def apodization_function(self):
        return str(self.apodization.currentText())

    @property
    def decay_constant(self):
        return float(self.decay.text())

    @property
    def negative_padding(self):
        return self.negativePadding.checkState()

    @property
    def padding_value(self):
        return int(self.padding.text())