from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *

 
def setName(table,row,name):
       text = QTableWidgetItem((name))
       text.setFlags(Qt.ItemIsEnabled)
       table.setItem(row,0, text)

def createComboTable(table,row,options):
    combo=QComboBox()
    combo.addItems(options)
    table.setCellWidget(row,1,combo )       
    return combo 

def createDoubleTable(table,value,row):
       numberWidget = QTableWidgetItem(str(value))
       table.setItem(row,1, numberWidget)
       return numberWidget

def createCheckTable(table,state,row):
	box = QTableWidgetItem()
        box.setFlags(Qt.ItemIsUserCheckable |Qt.ItemIsEnabled)
	if state:
	    box.setCheckState(QtCore.Qt.Checked)
	else:
	    box.setCheckState(QtCore.Qt.Unchecked)

        table.setItem(row,1, box)
        return box
