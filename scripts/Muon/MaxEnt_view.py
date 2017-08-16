from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui
from Muon import widget_helper


class MaxEntView(QtGui.QWidget):
    # signals
    maxEntButtonSignal = QtCore.pyqtSignal()
    #tableClickSignal = QtCore.pyqtSignal(object,object)

    def __init__(self, parent=None):
        super(MaxEntView, self).__init__(parent)
        self.grid = QtGui.QGridLayout(self)

        #make table
        self.table 	= QtGui.QTableWidget(self)
        self.table.setRowCount(8)
        self.table.setColumnCount(2)
        self.table.setColumnWidth(0,300)
        self.table.setColumnWidth(1,300)

        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.setHorizontalHeaderLabels(("MaxEnt Property;Value").split(";"))

        # populate table
        options=['test']

        widget_helper.setName(self.table,0,"Workspace")
        self.ws= widget_helper.createComboTable(self.table,0,options)
        
        widget_helper.setName(self.table,1,"Complex Data")
        self.complex_data_box= widget_helper.createCheckTable(self.table,False,1)

        widget_helper.setName(self.table,2,"Complex Image")
        self.complex_image_box= widget_helper.createCheckTable(self.table,True,2)
 
        widget_helper.setName(self.table,3,"Positive Image")
        self.positive_image_box= widget_helper.createCheckTable(self.table,False,3)
 
        widget_helper.setName(self.table,4,"Resolution")
        self.resolution_box= widget_helper.createSpinTable(self.table,1,4)

        widget_helper.setName(self.table,5,"Maximum entropy constant (A)")
        self.AConst= widget_helper.createDoubleTable(self.table,0.4,5)

        widget_helper.setName(self.table, 6,"Auto shift")
        self.shift_box= widget_helper.createCheckTable(self.table,False,6)

        widget_helper.setName(self.table, 7,"Raw")
        self.raw_box= widget_helper.createCheckTable(self.table,False,7)

        self.table.resizeRowsToContents()

        # advanced options table
        self.advancedLabel=QtGui.QLabel("\n  Advanced Options")
        #make table
        self.tableA 	= QtGui.QTableWidget(self)
        self.tableA.setRowCount(6)
        self.tableA.setColumnCount(2)
        self.tableA.setColumnWidth(0,300)
        self.tableA.setColumnWidth(1,300)
        self.tableA.horizontalHeader().setStretchLastSection(True)

        self.tableA.setHorizontalHeaderLabels(("MaxEnt Property;Value").split(";"))


        widget_helper.setName(self.tableA,0,"Chi target")
        self.chiTarget= widget_helper.createDoubleTable(self.tableA,100,0)

        widget_helper.setName(self.tableA,1,"Chi (precision)")
        self.chiEps= widget_helper.createDoubleTable(self.tableA,0.001,1)

        widget_helper.setName(self.tableA,2,"Distance Penalty")
        self.dist= widget_helper.createDoubleTable(self.tableA,0.1,2)

        widget_helper.setName(self.tableA,3,"Maximum Angle")
        self.angle= widget_helper.createDoubleTable(self.tableA,0.05,3)

        widget_helper.setName(self.tableA,4,"Max Iterations")
        self.max_iterations= widget_helper.createSpinTable(self.tableA,20000,4)

        widget_helper.setName(self.tableA,5,"Alpha Chop Iterations")
        self.chop= widget_helper.createSpinTable(self.tableA,500,5)

        #layout
        self.table.setMinimumSize(40,228)
        self.tableA.setMinimumSize(40,207)
        self.horizontalSpacer1 = QtGui.QSpacerItem(20, 30, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        self.horizontalSpacer2 = QtGui.QSpacerItem(20, 70, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        #make button
        self.button = QtGui.QPushButton('Calculate MaxEnt', self)
        self.button.setStyleSheet("background-color:lightgrey")
#        #connects
#        self.table.cellClicked.connect(self.tableClick)
        self.button.clicked.connect(self.MaxEntButtonClick)
        # add to layout
        self.grid.addWidget(self.table)
        self.grid.addItem(self.horizontalSpacer1)
        self.grid.addWidget(self.advancedLabel)
        self.grid.addWidget(self.tableA)
        self.grid.addItem(self.horizontalSpacer2)
        self.grid.addWidget(self.button)

    # add data to view
    def addItems(self,options):
        self.ws.clear()
        self.ws.addItems(options)
#
#    # connect signals
#    def tableClick(self,row,col):
#        self.tableClickSignal.emit(row,col)
#
    def MaxEntButtonClick(self):
        self.maxEntButtonSignal.emit()
#
#    #functions
#    def changed(self,box,row ):
#        self.table.setRowHidden(row,box.checkState() == QtCore.Qt.Checked)
#
#    def changedHideUnTick(self,box,row ):
#        self.table.setRowHidden(row, box.checkState() != QtCore.Qt.Checked)
#
    def initMaxEntInput(self):
        inputs={}
        inputs['InputWorkspace']=str( self.ws.currentText()).replace(";","; ")
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

        out=str( self.ws.currentText()).replace(";","; ")
        inputs['EvolChi']=out+";EvolChi;MaxEnt"
        inputs['EvolAngle']=out+";EvolAngle;MaxEnt"
        inputs['ReconstructedImage']=out+";ReconstructedImage;MaxEnt"
        inputs['ReconstructedData']=out+";ReconstructedData;MaxEnt"
              
        return inputs

#    def addFFTComplex(self,inputs):
#        inputs["InputImagWorkspace"]=str( self.Im_ws.currentText()).replace(";","; ")
#        inputs["Imaginary"] = 0 #always zero
#
#    def addFFTShift(self,inputs):
#        inputs['AutoShift']=False
#        inputs['Shift']= float(self.shift.text())
#
    def addRaw(self,inputs,key):
        inputs[key]+="_Raw"
#
#    # get methods
#    def isAutoShift(self):
#        return self.shift_box.checkState() == QtCore.Qt.Checked
#
#    def isComplex(self):
#        return self.Im_box.checkState() == QtCore.Qt.Checked

    def isRaw(self):
        return self.raw_box.checkState() == QtCore.Qt.Checked
#
#    def getImBoxRow(self):
#        return self.Im_box_row
#
#    def getShiftBoxRow(self):
#        return self.shift_box_row
#
#    def getImBox(self):
#        return self.Im_box
#
#    def getShiftBox(self):
#        return self.shift_box
