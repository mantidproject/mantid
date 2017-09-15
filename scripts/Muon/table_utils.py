from __future__ import (absolute_import, division, print_function)
from PyQt4 import QtCore, QtGui

"""
This module contains the methods for
adding information to tables.
"""


def setRowName(table,row,name):
    text = QtGui.QTableWidgetItem((name))
    text.setFlags(QtCore.Qt.ItemIsEnabled)
    table.setItem(row,0, text)


def addComboToTable(table,row,options):
    combo=QtGui.QComboBox()
    combo.addItems(options)
    table.setCellWidget(row,1,combo)
    return combo


def addDoubleToTable(table,value,row):
    numberWidget = QtGui.QTableWidgetItem(str(value))
    table.setItem(row,1, numberWidget)
    return numberWidget


def addCheckBoxToTable(table,state,row):
    box = QtGui.QTableWidgetItem()
    box.setFlags(QtCore.Qt.ItemIsUserCheckable |QtCore.Qt.ItemIsEnabled)
    if state:
        box.setCheckState(QtCore.Qt.Checked)
    else:
        box.setCheckState(QtCore.Qt.Unchecked)

    table.setItem(row,1, box)
    return box


def addSpinBoxToTable(table,default,row):
    box = QtGui.QSpinBox()
    if default > 99:
        box.setMaximum(default*10)
    box.setValue(default)
    table.setCellWidget(row,1,box)
    return box
