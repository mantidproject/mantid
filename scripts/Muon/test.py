from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
import sys

import mantid.simpleapi as mantid
from Muon import widgetHelper


class Form(QDialog):
     def __init__(self, parent=None):
            super(Form, self).__init__(parent)
	    self.table 	= QTableWidget()
	    self.tableItem 	= QTableWidgetItem()
	    # initiate table
	    self.table.resize(800, 800)
	    self.table.setRowCount(6)
	    self.table.setColumnCount(2)
	    self.table.setColumnWidth(0,300)
	    self.table.setColumnWidth(1,300)

            self.table.setHorizontalHeaderLabels(("Property;Value").split(";"))
	    # set data

	    options=str(mantid.AnalysisDataService.getObjectNames()).replace(" ", "")
	    options=options[2:-2] 
            options=options.split("','")
            print(options)  
	    widgetHelper.setName(self.table,0,"Workspace")        
	    self.WS= widgetHelper.createComboTable(self.table,0,options)        
 
            widgetHelper.setName(self.table,1,"Imaginary Workspace")        
	    self.ImWS= widgetHelper.createComboTable(self.table,1,options)        
 
            widgetHelper.setName(self.table,2,"Real Part")        
	    self.RePart= widgetHelper.createDoubleTable(self.table,0,2)        
 
            widgetHelper.setName(self.table,3,"Imagineary part")        
	    self.ImPart= widgetHelper.createDoubleTable(self.table,0,3)        
 
            widgetHelper.setName(self.table,4,"Auto shift")        
	    self.shiftBox= widgetHelper.createCheckTable(self.table,True,4)        
            self.table.cellClicked.connect(self.Clicked) 

            widgetHelper.setName(self.table,5,"Shift")        
	    self.shift= widgetHelper.createDoubleTable(self.table,0.0,5)        
	    self.table.hideRow(5)


	    		#self.table=self.createComboTable(self.keys[j],j,col)	

	   # for comoBox in self.comboBoxes:	
           #       self.connect(comboBox, QtCore.SIGNAL("currentIndexChanged(const QString&)"), self.updateValue)
            grid = QGridLayout()
            grid.addWidget(self.table)

            self.button = QPushButton('Calculate FFT', self)
            self.button.setStyleSheet("background-color:lightgrey")

            self.button.clicked.connect(self.handleButton)


            grid.addWidget(self.button)

            self.setLayout(grid)

 
     def Clicked(self,row,col):
         if row == 4 and col ==1:
		self.shiftChanged()
     
     def shiftChanged(self):
      
        if self.shiftBox.checkState() == QtCore.Qt.Checked:
            self.table.hideRow(5)
        else:
            self.table.showRow(5)

     def handleButton(self):

        if self.shiftBox.checkState() == QtCore.Qt.Checked:
           output=mantid.FFT(InputWorkspace=str( self.WS.currentText()),OutputWorkspace="MuonFFT",
                             InputImagWorkspace=str( self.ImWS.currentText()),Real=int(self.RePart.text()),
                             Imaginary=int(self.ImPart.text()),AutoShift=True,  
                             AcceptXRoundingErrors=True)

        else:
           output=mantid.FFT(InputWorkspace=str( self.WS.currentText()),OutputWorkspace="MuonFFT",
                             InputImagWorkspace=str( self.ImWS.currentText()),Real=int(self.RePart.text()),
                             Imaginary=int(self.ImPart.text()),AutoShift=False,  
                             Shift=float(self.shift.text()),AcceptXRoundingErrors=True)



#        a =        print (a)[1;5B
#        print (a)
#        a =str( self.RePart.text())
#        print (a)
#        a =str( self.ImPart.text())
#        print (a)
#        if self.shiftBox.checkState() == QtCore.Qt.Checked:
#	   a =str( self.ImPart.text())
#           print (self.shift.text())
# 	
#	print ("done")
