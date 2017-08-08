from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
import sys

import mantid.simpleapi as mantid
from Muon import widgetHelper


class FFTView(QDialog):
     # signals
     button_signal = pyqtSignal() 
     table_click_signal = pyqtSignal(object,object)
 
     def __init__(self, parent=None):
            super(FFTView, self).__init__(parent)
	    self.table 	= QTableWidget()
	    self.tableItem 	= QTableWidgetItem()
	    # initiate table
	    self.table.resize(800, 800)
	    self.table.setRowCount(6)
	    self.table.setColumnCount(2)
	    self.table.setColumnWidth(0,300)
	    self.table.setColumnWidth(1,300)

            self.table.setHorizontalHeaderLabels(("FFT Property;Value").split(";"))

	    # set data
            options=['test']
         
	    widgetHelper.setName(self.table,0,"Workspace")        
	    self.WS= widgetHelper.createComboTable(self.table,0,options)        
 
            self.ImBoxRow=1
            widgetHelper.setName(self.table,self.ImBoxRow,"Imaginary Data")        
	    self.ImBox= widgetHelper.createCheckTable(self.table,True,self.ImBoxRow)        
 
            widgetHelper.setName(self.table,2,"Imaginary Workspace")        
	    self.ImWS= widgetHelper.createComboTable(self.table,2,options)        
            
            self.shiftBoxRow=3     
            widgetHelper.setName(self.table,self.shiftBoxRow,"Auto shift")        
	    self.shiftBox= widgetHelper.createCheckTable(self.table,True,self.shiftBoxRow)        

            widgetHelper.setName(self.table,4,"Shift")        
	    self.shift= widgetHelper.createDoubleTable(self.table,0.0,4)        
	    self.table.hideRow(4)
 
            widgetHelper.setName(self.table,5,"Use Raw data")        
	    self.RawBox= widgetHelper.createCheckTable(self.table,True,5)        



            self.button = QPushButton('Calculate FFT', self)
            self.button.setStyleSheet("background-color:lightgrey")
 

            #connects
            self.table.cellClicked.connect(self.tableClick)
            self.button.clicked.connect(self.buttonClick)

            self.grid = QGridLayout()
            self.grid.addWidget(self.table)
            self.grid.addWidget(self.button)
     
     # add data to view
     def addItems(self,options):
         self.WS.clear()  
         self.WS.addItems(options)  
         self.ImWS.clear()  
         self.ImWS.addItems(options) 
 
     # connect signals
     def tableClick(self,row,col):
          self.table_click_signal.emit(row,col)

     def buttonClick(self):
          self.button_signal.emit()
 
    # functions
     def changed(self,box,row ):
        if box.checkState() == QtCore.Qt.Checked:
            self.table.setRowHidden(row,True)
        else:
            self.table.setRowHidden(row,False)

     def changedHideUnTick(self,box,row ):
      
        if box.checkState() == QtCore.Qt.Checked:
            self.table.setRowHidden(row,False)
        else:
            self.table.setRowHidden(row,True)


     def init_FFT_input(self):
        inputs={}
        inputs['InputWorkspace']=str( self.WS.currentText()).replace(";","; ")
        inputs['Real']= 0 # always zero 
        inputs['OutputWorkspace']="MuonFFT"
        inputs["AcceptXRoundingErrors"]=True
        return inputs

     def add_FFT_complex(self,inputs):
         inputs["InputImagWorkspace"]=str( self.ImWS.currentText()).replace(";","; ")
         inputs["Imaginary"] = 0 #always zero
 
     def add_FFT_shift(self,inputs): 
             inputs['AutoShift']=False
             inputs['Shift']= float(self.shift.text())
 
     def isAutoShift(self):
        return self.shiftBox.checkState() == QtCore.Qt.Checked

     def isComplex(self):
        return self.ImBox.checkState() == QtCore.Qt.Checked
     def isRaw(self):   
        return self.RawBox.checkState() == QtCore.Qt.Checked    
     def getImBoxRow(self):   
        return self.ImBoxRow
     def getShiftBoxRow(self):   
        return self.shiftBoxRow
