from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui

from Muon import table_utils


class MaxEntView(QtGui.QWidget):
    """
    The view for the MaxEnt widget. This
    creates the look of the widget
    """
    # signals
    maxEntButtonSignal = QtCore.pyqtSignal()
    cancelSignal = QtCore.pyqtSignal()

    def __init__(self, parent=None):
        super(MaxEntView, self).__init__(parent)
        self.grid = QtGui.QVBoxLayout(self)

        #make table
        self.table = QtGui.QTableWidget(self)
        self.table.resize(800, 800)

        self.table.setRowCount(8)
        self.table.setColumnCount(2)
        self.table.setColumnWidth(0,300)
        self.table.setColumnWidth(1,300)
        self.table.verticalHeader().setVisible(False)
        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.setHorizontalHeaderLabels(("MaxEnt Property;Value").split(";"))
        table_utils.setTableHeaders(self.table)

        # populate table
        table_utils.setRowName(self.table,0,"Complex Data")
        self.complex_data_box= table_utils.addCheckBoxToTable(self.table,False,0)
        self.complex_data_box.setFlags(QtCore.Qt.ItemIsEnabled)
        # needs an even number of ws to work
        # so lets hide it for now
        self.table.setRowHidden(0,True)

        table_utils.setRowName(self.table,1,"Complex Image")
        self.complex_image_box= table_utils.addCheckBoxToTable(self.table,True,1)
        table_utils.setRowName(self.table,2,"Positive Image")
        self.positive_image_box= table_utils.addCheckBoxToTable(self.table,False,2)
        table_utils.setRowName(self.table,3,"Resolution")
        self.resolution_box= table_utils.addSpinBoxToTable(self.table,1,3)

        table_utils.setRowName(self.table,4,"Maximum entropy constant (A)")
        self.AConst= table_utils.addDoubleToTable(self.table,0.4,4)

        table_utils.setRowName(self.table, 5,"Auto shift")
        self.shift_box= table_utils.addCheckBoxToTable(self.table,False,5)

        table_utils.setRowName(self.table, 6,"Raw")
        self.raw_box= table_utils.addCheckBoxToTable(self.table,False,6)

        # this will be removed once maxEnt does a simultaneous fit
        options=['test']
        table_utils.setRowName(self.table,7,"Workspace")
        self.ws= table_utils.addComboToTable(self.table,7,options)

        self.table.resizeRowsToContents()

        # advanced options table
        self.advancedLabel=QtGui.QLabel("\n  Advanced Options")
        #make table
        self.tableA = QtGui.QTableWidget(self)
        self.tableA.resize(800, 800)

        self.tableA.setRowCount(6)
        self.tableA.setColumnCount(2)
        self.tableA.setColumnWidth(0,300)
        self.tableA.setColumnWidth(1,300)

        self.tableA.verticalHeader().setVisible(False)
        self.tableA.horizontalHeader().setStretchLastSection(True)

        self.tableA.setHorizontalHeaderLabels(("Advanced Property;Value").split(";"))
        table_utils.setTableHeaders(self.tableA)

        table_utils.setRowName(self.tableA,0,"Chi target")
        self.chiTarget= table_utils.addDoubleToTable(self.tableA,100,0)

        table_utils.setRowName(self.tableA,1,"Chi (precision)")
        self.chiEps= table_utils.addDoubleToTable(self.tableA,0.001,1)

        table_utils.setRowName(self.tableA,2,"Distance Penalty")
        self.dist= table_utils.addDoubleToTable(self.tableA,0.1,2)

        table_utils.setRowName(self.tableA,3,"Maximum Angle")
        self.angle= table_utils.addDoubleToTable(self.tableA,0.05,3)

        table_utils.setRowName(self.tableA,4,"Max Iterations")
        self.max_iterations= table_utils.addSpinBoxToTable(self.tableA,20000,4)

        table_utils.setRowName(self.tableA,5,"Alpha Chop Iterations")
        self.chop= table_utils.addSpinBoxToTable(self.tableA,500,5)

        #layout
        # this is if complex data is unhidden
        self.table.setMinimumSize(40,203)
        self.tableA.setMinimumSize(40,207)
        self.horizontalSpacer1 = QtGui.QSpacerItem(20, 30, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        self.horizontalSpacer2 = QtGui.QSpacerItem(20, 70, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        #make buttons
        self.button = QtGui.QPushButton('Calculate MaxEnt', self)
        self.button.setStyleSheet("background-color:lightgrey")
        self.cancel = QtGui.QPushButton('Cancel', self)
        self.cancel.setStyleSheet("background-color:lightgrey")
        self.cancel.setEnabled(False)
        #connects
        self.button.clicked.connect(self.MaxEntButtonClick)
        self.cancel.clicked.connect(self.cancelClick)
        # button layout
        self.buttonLayout=QtGui.QHBoxLayout()
        self.buttonLayout.addWidget(self.button)
        self.buttonLayout.addWidget(self.cancel)
        # add to layout
        self.grid.addWidget(self.table)
        self.grid.addItem(self.horizontalSpacer1)
        self.grid.addWidget(self.advancedLabel)
        self.grid.addWidget(self.tableA)
        self.grid.addItem(self.horizontalSpacer2)
        self.grid.addLayout(self.buttonLayout)

    # add data to view
    def addItems(self,options):
        self.ws.clear()
        self.ws.addItems(options)

    # send signal
    def MaxEntButtonClick(self):
        self.maxEntButtonSignal.emit()

    def cancelClick(self):
        self.cancelSignal.emit()

    # get some inputs for model
    def initMaxEntInput(self):
        inputs={}

        #  this will be removed once maxEnt does a simultaneous fit
        inputs['InputWorkspace']=str( self.ws.currentText()).replace(";","; ")
        # will use this instead of the above
        #inputs['InputWorkspace']="MuonAnalysis"
        inputs['ComplexData']=  self.complex_data_box.checkState()
        inputs["ComplexImage"]=  self.complex_image_box.checkState()
        inputs['PositiveImage']=self.positive_image_box.checkState()
        inputs["ResolutionFactor"]=int(self.resolution_box.text())
        inputs["A"] = float(self.AConst.text())
        inputs["AutoShift"]=self.shift_box.checkState()
        inputs["ChiTarget"]=float(self.chiTarget.text())
        inputs["ChiEps"]=float(self.chiEps.text())
        inputs["DistancePenalty"]=float(self.dist.text())
        inputs["MaxAngle"]=float(self.angle.text())
        inputs["MaxIterations"]=int(self.max_iterations.text())
        inputs["AlphaChopIterations"]=int(self.chop.text())

        # will remove this when sim maxent Works
        out=str( self.ws.currentText()).replace(";","; ")

        inputs['EvolChi']=out+";EvolChi;MaxEnt"
        inputs['EvolAngle']=out+";EvolAngle;MaxEnt"
        inputs['ReconstructedImage']=out+";FrequencyDomain;MaxEnt"
        inputs['ReconstructedData']=out+";TimeDomain;MaxEnt"

        return inputs

    def addRaw(self,inputs,key):
        inputs[key]+="_Raw"

    def isRaw(self):
        return self.raw_box.checkState() == QtCore.Qt.Checked

    # turn button on and off
    def activateCalculateButton(self):
        self.button.setEnabled(True)
        self.cancel.setEnabled(False)

    def deactivateCalculateButton(self):
        self.button.setEnabled(False)
        self.cancel.setEnabled(True)
