from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui
from Muon import widget_helper


class FFTView(QtGui.QWidget):
    # signals
    buttonSignal = QtCore.pyqtSignal()
    tableClickSignal = QtCore.pyqtSignal(object,object)

    def __init__(self, parent=None):
        super(FFTView, self).__init__(parent)
        self.grid = QtGui.QGridLayout(self)
        self.FFTTable 	= QtGui.QTableWidget(self)

        #make table
        self.FFTTable.resize(800, 800)
        self.FFTTable.setRowCount(6)
        self.FFTTable.setColumnCount(2)
        self.FFTTable.setColumnWidth(0,300)
        self.FFTTable.setColumnWidth(1,300)
        self.FFTTable.verticalHeader().setVisible(False)
        self.FFTTable.horizontalHeader().setStretchLastSection(True)
        self.FFTTable.setHorizontalHeaderLabels(("FFT Property;Value").split(";"))

        # populate table
        options=['test']

        widget_helper.setName(self.FFTTable,0,"Workspace")
        self.ws= widget_helper.createComboTable(self.FFTTable,0,options)
        self.Im_box_row=1
        widget_helper.setName(self.FFTTable,self.Im_box_row,"Imaginary Data")
        self.Im_box= widget_helper.createCheckTable(self.FFTTable,True,self.Im_box_row)

        widget_helper.setName(self.FFTTable,2,"Imaginary Workspace")
        self.Im_ws= widget_helper.createComboTable(self.FFTTable,2,options)

        self.shift_box_row=3
        widget_helper.setName(self.FFTTable,self.shift_box_row,"Auto shift")
        self.shift_box= widget_helper.createCheckTable(self.FFTTable,True,self.shift_box_row)

        widget_helper.setName(self.FFTTable,4,"Shift")
        self.shift= widget_helper.createDoubleTable(self.FFTTable,0.0,4)
        self.FFTTable.hideRow(4)

        widget_helper.setName(self.FFTTable,5,"Use Raw data")
        self.Raw_box= widget_helper.createCheckTable(self.FFTTable,True,5)

        self.FFTTable.resizeRowsToContents()
        #make advanced table options
        self.advancedLabel=QtGui.QLabel("\n Advanced Options")
        self.FFTTableA     = QtGui.QTableWidget(self)
        self.FFTTableA.setRowCount(4)
        self.FFTTableA.setColumnCount(2)
        self.FFTTableA.setColumnWidth(0,300)
        self.FFTTableA.setColumnWidth(1,300)
        self.FFTTableA.horizontalHeader().setStretchLastSection(True)
        self.FFTTableA.verticalHeader().setVisible(False)
        self.FFTTableA.setHorizontalHeaderLabels(("PreProcessing Property;Value").split(";"))

        widget_helper.setName(self.FFTTableA,0,"Apodization Function")
        options=["None","Lorentz","Gaussian"]
        self.apodization = widget_helper.createComboTable(self.FFTTableA,0,options)

        widget_helper.setName(self.FFTTableA,1,"Decay Constant (micro seconds)")
        self.decay = widget_helper.createDoubleTable(self.FFTTableA,1.4,1)

        widget_helper.setName(self.FFTTableA,2,"Negative Padding")
        self.negativePadding= widget_helper.createCheckTable(self.FFTTableA,True,2)

        widget_helper.setName(self.FFTTableA,3,"Padding")
        self.padding= widget_helper.createSpinTable(self.FFTTableA,1,3)
        self.FFTTableA.resizeRowsToContents()

        #make button
        self.button = QtGui.QPushButton('Calculate FFT', self)
        self.button.setStyleSheet("background-color:lightgrey")
        #connects
        self.FFTTable.cellClicked.connect(self.tableClick)
        self.button.clicked.connect(self.buttonClick)
        # add to layout
        self.FFTTable.setMinimumSize(40,158)
        self.FFTTableA.setMinimumSize(40,127)
        self.horizontalSpacer1 = QtGui.QSpacerItem(20, 94, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        self.horizontalSpacer2 = QtGui.QSpacerItem(20, 280, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        # add to layout
        self.grid.addWidget(self.FFTTable)
        self.grid.addItem(self.horizontalSpacer1)
        self.grid.addWidget(self.advancedLabel)
        self.grid.addWidget(self.FFTTableA)
        self.grid.addItem(self.horizontalSpacer2)
        self.grid.addWidget(self.button)

   # add data to view
    def addItems(self,options):
        self.ws.clear()
        self.ws.addItems(options)
        self.Im_ws.clear()
        self.Im_ws.addItems(options)

    # connect signals
    def tableClick(self,row,col):
        self.tableClickSignal.emit(row,col)

    def buttonClick(self):
        self.buttonSignal.emit()

    #functions
    def changed(self,box,row ):
        self.FFTTable.setRowHidden(row,box.checkState() == QtCore.Qt.Checked)

    def changedHideUnTick(self,box,row ):
        self.FFTTable.setRowHidden(row, box.checkState() != QtCore.Qt.Checked)

    def initFFTInput(self):
        inputs={}
        inputs['InputWorkspace']="__ReTmp__"#str( self.ws.currentText()).replace(";","; ")
        inputs['Real']= 0 # always zero
        out=str( self.ws.currentText()).replace(";","; ")
        inputs['OutputWorkspace']=out+";FFT"
        inputs["AcceptXRoundingErrors"]=True
        return inputs

    def addFFTComplex(self,inputs):
        inputs["InputImagWorkspace"]="__ImTmp__"#str( self.Im_ws.currentText()).replace(";","; ")
        inputs["Imaginary"] = 0 #always zero

    def addFFTShift(self,inputs):
        inputs['AutoShift']=False
        inputs['Shift']= float(self.shift.text())

    def addRaw(self,inputs,key):
        inputs[key]+="_Raw"

    def initAdvanced(self):
        inputs={}
        inputs["ApodizationFunction"]=str(self.apodization.currentText())
        inputs["DecayConstant"]=float(self.decay.text())
        inputs["NegativePadding"] = self.negativePadding.checkState()
        inputs["Padding"]=int(self.padding.text())
        return inputs

    def ReAdvanced(self,inputs):
        inputs['InputWorkspace']=str( self.ws.currentText()).replace(";","; ")
        inputs['OutputWorkspace']="__ReTmp__"

    def ImAdvanced(self,inputs):
        inputs['InputWorkspace']=str( self.Im_ws.currentText()).replace(";","; ")
        inputs['OutputWorkspace']="__ImTmp__"

    # get methods
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
