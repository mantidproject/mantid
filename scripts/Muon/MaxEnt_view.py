from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui
from Muon import widget_helper


class MaxEntView(QtGui.QWidget):
    # signals
    #buttonSignal = QtCore.pyqtSignal()
    #tableClickSignal = QtCore.pyqtSignal(object,object)

    def __init__(self, parent=None):
        super(MaxEntView, self).__init__(parent)
        self.grid = QtGui.QGridLayout(self)
        self.table 	= QtGui.QTableWidget(self)

        #make table
        self.table.resize(800, 800)
        self.table.setRowCount(6)
        self.table.setColumnCount(2)
        self.table.setColumnWidth(0,300)
        self.table.setColumnWidth(1,300)

        self.table.setHorizontalHeaderLabels(("MaxEnt Property;Value").split(";"))

        # populate table
        options=['test']

        widget_helper.setName(self.table,0,"Workspace")
        self.ws= widget_helper.createComboTable(self.table,0,options)
#        self.Im_box_row=1
#
#        widget_helper.setName(self.table,self.Im_box_row,"Imaginary Data")
#        self.Im_box= widget_helper.createCheckTable(self.table,True,self.Im_box_row)
#
#        widget_helper.setName(self.table,2,"Imaginary Workspace")
#        self.Im_ws= widget_helper.createComboTable(self.table,2,options)
#
#        self.shift_box_row=3
#        widget_helper.setName(self.table,self.shift_box_row,"Auto shift")
#        self.shift_box= widget_helper.createCheckTable(self.table,True,self.shift_box_row)
#
#        widget_helper.setName(self.table,4,"Shift")
#        self.shift= widget_helper.createDoubleTable(self.table,0.0,4)
#        self.table.hideRow(4)
#
#        widget_helper.setName(self.table,5,"Use Raw data")
#        self.Raw_box= widget_helper.createCheckTable(self.table,True,5)
#        #make button
#        self.button = QtGui.QPushButton('Calculate FFT', self)
#        self.button.setStyleSheet("background-color:lightgrey")
#        #connects
#        self.table.cellClicked.connect(self.tableClick)
#        self.button.clicked.connect(self.buttonClick)
#        # add to layout
        self.grid.addWidget(self.table)
#        self.grid.addWidget(self.button)

#    # add data to view
#    def addItems(self,options):
#        self.ws.clear()
#        self.ws.addItems(options)
#        self.Im_ws.clear()
#        self.Im_ws.addItems(options)
#
#    # connect signals
#    def tableClick(self,row,col):
#        self.tableClickSignal.emit(row,col)
#
#    def buttonClick(self):
#        self.buttonSignal.emit()
#
#    #functions
#    def changed(self,box,row ):
#        self.table.setRowHidden(row,box.checkState() == QtCore.Qt.Checked)
#
#    def changedHideUnTick(self,box,row ):
#        self.table.setRowHidden(row, box.checkState() != QtCore.Qt.Checked)
#
#    def initFFTInput(self):
#        inputs={}
#        inputs['InputWorkspace']=str( self.ws.currentText()).replace(";","; ")
#        inputs['Real']= 0 # always zero
#        out=str( self.ws.currentText()).replace(";","; ")
#        inputs['OutputWorkspace']=out+";FFT"
#        inputs["AcceptXRoundingErrors"]=True
#        return inputs
#
#    def addFFTComplex(self,inputs):
#        inputs["InputImagWorkspace"]=str( self.Im_ws.currentText()).replace(";","; ")
#        inputs["Imaginary"] = 0 #always zero
#
#    def addFFTShift(self,inputs):
#        inputs['AutoShift']=False
#        inputs['Shift']= float(self.shift.text())
#
#    def addRaw(self,inputs,key):
#        inputs[key]+="_Raw"
#
#    # get methods
#    def isAutoShift(self):
#        return self.shift_box.checkState() == QtCore.Qt.Checked
#
#    def isComplex(self):
#        return self.Im_box.checkState() == QtCore.Qt.Checked
#
#    def isRaw(self):
#        return self.Raw_box.checkState() == QtCore.Qt.Checked
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
