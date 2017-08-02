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
	    self.table.resize(400, 250)
	    self.table.setRowCount(6)
	    self.table.setColumnCount(2)
            self.table.setHorizontalHeaderLabels(("Property;Value").split(";"))
	    # set data

	    options=str(mantid.AnalysisDataService.getObjectNames()).split("',  '")        
	    widgetHelper.setName(self.table,0,"Workspace")        
	    self.WS= widgetHelper.createComboTable(self.table,0,options)        
 
            widgetHelper.setName(self.table,1,"Imaginary Workspace")        
	    self.ImWS= widgetHelper.createComboTable(self.table,1,options)        
 
            widgetHelper.setName(self.table,2,"Real Part")        
	    self.RePart= widgetHelper.createDoubleTable(self.table,0.0,2)        
 
            widgetHelper.setName(self.table,3,"Imagineary part")        
	    self.ImPart= widgetHelper.createDoubleTable(self.table,0.0,3)        
 
            widgetHelper.setName(self.table,4,"Auto shift")        
	    self.shiftBox= widgetHelper.createCheckTable(self.table,True,4)        
            self.table.cellClicked.connect(self.Clicked) 

            widgetHelper.setName(self.table,5,"Shift")        
	    self.shift= widgetHelper.createDoubleTable(self.table,0.0,5)        
	   


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

 

     def handleButton(self):
        dic ={'fwd': 1.0, 'bwd':2.0,'long':3.0}
        group1 = dic[str(self.group1[self.active_row].currentText())]
        group2 = dic[str(self.group2[self.active_row].currentText())]
	alpha = self.calcAlpha(group1,group2)
        self.setAlpha(self.active_row,alpha)

   
     def setAlpha(self,row,value):
            self.table.setItem(row,3, QTableWidgetItem(str(value)))
	    return self.table	

     def calcAlpha(self,group1,group2):
	 return group1/group2   	

     def Clicked(self,row,col):
         if row == 4 and col ==1:
		self.shiftChanged()
     
     def shiftChanged(self):
      
        if self.shiftBox.checkState() == QtCore.Qt.Checked:
            self.table.showRow(5)
        else:
            self.table.hideRow(5)
