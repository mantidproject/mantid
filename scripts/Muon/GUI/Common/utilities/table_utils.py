# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import os
from functools import wraps
import sys
from qtpy import QtWidgets, QtCore, QtGui

"""
This module contains the methods for
adding information to tables.
"""


class ValidatedTableItem(QtWidgets.QTableWidgetItem):
    """
    An extension of the QTableWidgetItem class, which modifies the setData method to first check that the entered
    text is valid; and only runs setData if the validator returns True.

    Essentially functions identically to the QTableWidgetItem, but only sets valid entries. If no validator is supplied
    then the behaviour is identical.

    Example usage:

    def greater_than_zero_validator(string_val):
        return float(string_val) > 0

    table_item = ValidatedTableItem(greater_than_zero_validator)
    """

    @staticmethod
    def validator_before_set(func, validator):
        @wraps(func)
        def wrapper(*args, **kw):
            try:
                if isinstance(args[1], QtCore.QVariant):
                    # when standalone, type is QtCore.QVariant
                    validator_arg = args[1].toString()
                else:
                    # within MantidPlot, type is unicode
                    validator_arg = args[1]

                if validator(validator_arg):
                    res = func(*args, **kw)
                else:
                    res = None
                return res
            except Exception as e:
                print("EXCEPTION FROM ValidatedTableItem : ", e.args[0])

        return wrapper

    @staticmethod
    def default_validator(_text):
        return True

    def __init__(self, validator=default_validator):
        """
        :param validator: A predicate function (returns True/False) taking a single string as argument
        """
        super(ValidatedTableItem, self).__init__(0)
        self._validator = validator
        self._modify_setData()

    def validator(self, text):
        return self._validator(text)

    def _modify_setData(self):
        """
        Modify the setData method.
        """
        setattr(self, "setData", self.validator_before_set(self.setData, self.validator))


def setRowName(table, row, name):
    text = QtWidgets.QTableWidgetItem((name))
    text.setFlags(QtCore.Qt.ItemIsEnabled)
    table.setItem(row, 0, text)


def addComboToTable(table,row,options,col=1):
    combo=QtWidgets.QComboBox()
    combo.addItems(options)
    table.setCellWidget(row,col,combo)
    return combo


def addDoubleToTable(table,value,row,col=1, minimum=0.0):
    number_widget = QtWidgets.QLineEdit(str(value))
    number_widget.setValidator(QtGui.QDoubleValidator(minimum, sys.float_info.max, 3))
    table.setCellWidget(row,col, number_widget)
    return number_widget


def addCheckBoxToTable(table,state,row,col=1):
    box = QtWidgets.QTableWidgetItem()
    box.setFlags(QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled)
    if state:
        box.setCheckState(QtCore.Qt.Checked)
    else:
        box.setCheckState(QtCore.Qt.Unchecked)

    table.setItem(row,col, box)
    return box


def addCheckBoxWidgetToTable(table,state,row,col=1):
    check_box_widget = QtWidgets.QWidget()
    layout = QtWidgets.QHBoxLayout(check_box_widget)
    layout.setAlignment(QtCore.Qt.AlignCenter)
    layout.setContentsMargins(0, 0, 0, 0)
    box = QtWidgets.QCheckBox()
    box.setChecked(state)

    layout.addWidget(box)

    table.setCellWidget(row,col, check_box_widget)
    return box


def addSpinBoxToTable(table,default,row,col=1):
    box = QtWidgets.QSpinBox()
    if default > 99:
        box.setMaximum(default * 10)
    box.setValue(default)
    table.setCellWidget(row,col,box)
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
