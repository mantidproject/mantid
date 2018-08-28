from __future__ import (absolute_import, division, print_function)

from qtpy import QtCore, QtWidgets
from qtpy.QtCore import Signal

import mantid.simpleapi as mantid

from Muon.GUI.Common import table_utils


class FFTView(QtWidgets.QWidget):

    """
    creates the layout for the FFT GUI
    """
    # signals
    buttonSignal = Signal()
    tableClickSignal = Signal(object, object)
    phaseCheckSignal = Signal()

    def __init__(self, parent=None):
        super(FFTView, self).__init__(parent)
        self.grid = QtWidgets.QGridLayout(self)

        # add splitter for resizing
        splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical)

        # make table
        self.FFTTable = QtWidgets.QTableWidget(self)
        self.FFTTable.resize(800, 800)
        self.FFTTable.setRowCount(9)
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

        table_utils.setRowName(self.FFTTable, 6, "First Good Data")
        self.x0 = table_utils.addDoubleToTable(self.FFTTable, 0.1, 6)
        self.FFTTable.hideRow(6)

        table_utils.setRowName(self.FFTTable, 7, "Last Good Data")
        self.xN = table_utils.addDoubleToTable(self.FFTTable, 15.0, 7)
        self.FFTTable.hideRow(7)

        table_utils.setRowName(self.FFTTable, 8, "Construct Phase Table")
        self.phaseTable_box = table_utils.addCheckBoxToTable(
            self.FFTTable, True, 8)
        self.FFTTable.hideRow(8)

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
        options = ["None", "Lorentz", "Gaussian"]
        self.apodization = table_utils.addComboToTable(
            self.FFTTableA, 0, options)

        table_utils.setRowName(
            self.FFTTableA,
            1,
            "Decay Constant (micro seconds)")
        self.decay = table_utils.addDoubleToTable(self.FFTTableA, 1.4, 1)

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
        self.ws.addItem("PhaseQuad")
        self.Im_ws.clear()
        self.Im_ws.addItems(options)
        self.phaseQuadChanged()

    # connect signals
    def phaseCheck(self):
        self.phaseCheckSignal.emit()

    def tableClick(self, row, col):
        self.tableClickSignal.emit(row, col)

    def buttonClick(self):
        self.buttonSignal.emit()

    # responses to commands
    def activateButton(self):
        self.button.setEnabled(True)

    def deactivateButton(self):
        self.button.setEnabled(False)

    def setPhaseBox(self):
        self.FFTTable.setRowHidden(8, self.getWS() != "PhaseQuad")

    def changed(self, box, row):
        self.FFTTable.setRowHidden(row, box.checkState() == QtCore.Qt.Checked)

    def changedHideUnTick(self, box, row):
        self.FFTTable.setRowHidden(row, box.checkState() != QtCore.Qt.Checked)

    def phaseQuadChanged(self):
        # show axis
        self.FFTTable.setRowHidden(6, self.getWS() != "PhaseQuad")
        self.FFTTable.setRowHidden(7, self.getWS() != "PhaseQuad")
        # hide complex ws
        self.FFTTable.setRowHidden(2, self.getWS() == "PhaseQuad")

    # these are for getting inputs
    def getRunName(self):
        if mantid.AnalysisDataService.doesExist("MuonAnalysis_1"):
            tmpWS = mantid.AnalysisDataService.retrieve("MuonAnalysis_1")
        else:
            tmpWS = mantid.AnalysisDataService.retrieve("MuonAnalysis")
        return tmpWS.getInstrument().getName() + str(tmpWS.getRunNumber()).zfill(8)

    def initFFTInput(self):
        inputs = {}
        inputs[
            'InputWorkspace'] = "__ReTmp__"  # str( self.ws.currentText()).replace(";","; ")
        inputs['Real'] = 0  # always zero
        out = str(self.ws.currentText()).replace(";", "; ")
        inputs['OutputWorkspace'] = self.getRunName() + ";" + out + ";FFT"
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
