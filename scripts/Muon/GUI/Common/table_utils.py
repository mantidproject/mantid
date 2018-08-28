from __future__ import (absolute_import, division, print_function)
from qtpy import QtCore, QtWidgets
import os

"""
This module contains the methods for
adding information to tables.
"""


def setRowName(table, row, name):
    text = QtWidgets.QTableWidgetItem((name))
    text.setFlags(QtCore.Qt.ItemIsEnabled)
    table.setItem(row, 0, text)


def addComboToTable(table, row, options):
    combo = QtWidgets.QComboBox()
    combo.addItems(options)
    table.setCellWidget(row, 1, combo)
    return combo


def addDoubleToTable(table, value, row):
    numberWidget = QtWidgets.QTableWidgetItem(str(value))
    table.setItem(row, 1, numberWidget)
    return numberWidget


def addCheckBoxToTable(table, state, row):
    box = QtWidgets.QTableWidgetItem()
    box.setFlags(QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled)
    if state:
        box.setCheckState(QtCore.Qt.Checked)
    else:
        box.setCheckState(QtCore.Qt.Unchecked)

    table.setItem(row, 1, box)
    return box


def addSpinBoxToTable(table, default, row):
    box = QtWidgets.QSpinBox()
    if default > 99:
        box.setMaximum(default * 10)
    box.setValue(default)
    table.setCellWidget(row, 1, box)
    return box


# This is a work around a Windows 10
# bug that stops tables having underlines for
# the headers.
def setTableHeaders(table):
    # is it not windows
    if os.name != "nt":
        return
    version = QtCore.QSysInfo.WindowsVersion
    WINDOWS_10 = 160
    if (version == WINDOWS_10):
        styleSheet = \
            "QHeaderView::section{" \
            + "border-top:0px solid #D8D8D8;" \
            + "border-left:0px solid #D8D8D8;" \
            + "border-right:1px solid #D8D8D8;" \
            + "border-bottom: 1px solid #D8D8D8;" \
            + "background-color:white;" \
            + "padding:4px;" \
            + "}" \
            + "QTableCornerButton::section{" \
            + "border-top:0px solid #D8D8D8;" \
            + "border-left:0px solid #D8D8D8;" \
            + "border-right:1px solid #D8D8D8;" \
            + "border-bottom: 1px solid #D8D8D8;" \
            + "background-color:white;" \
            + "}"
        table.setStyleSheet(styleSheet)
        return styleSheet
    return
