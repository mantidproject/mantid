# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from functools import wraps
import sys
from qtpy import QtWidgets, QtCore
from mantidqt.utils.qt.line_edit_double_validator import LineEditDoubleValidator

"""
This module contains the methods for
adding information to tables.
"""


def default_validator(_text):
    return True


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

    def __init__(self, validator=default_validator):
        """
        :param validator: A predicate function (returns True/False) taking a single string as argument
        """
        super(ValidatedTableItem, self).__init__(0)
        self._validator = validator
        self._modify_setData()

    def validator(self, text):
        return self._validator(text)

    def set_validator(self, validator):
        self._validator = validator
        self._modify_setData()

    def _modify_setData(self):
        """
        Modify the setData method.
        """
        setattr(self, "setData", self.validator_before_set(self.setData, self.validator))


def setRowName(table, row, name, col=0):
    text = QtWidgets.QTableWidgetItem((name))
    text.setFlags(QtCore.Qt.ItemIsEnabled)
    table.setItem(row, col, text)


def set_table_item_flags(item: QtWidgets.QTableWidgetItem, editable: bool, enabled: bool) -> QtWidgets.QTableWidgetItem:
    if not editable:
        item.setFlags(item.flags() ^ QtCore.Qt.ItemIsEditable)
    if not enabled:
        item.setFlags(item.flags() ^ QtCore.Qt.ItemIsEnabled)
    return item


def create_string_table_item(text: str, editable: bool = True, enabled: bool = True, alignment: int = QtCore.Qt.AlignCenter) \
        -> QtWidgets.QTableWidgetItem:
    item = QtWidgets.QTableWidgetItem(text)
    item.setTextAlignment(alignment)
    item = set_table_item_flags(item, editable, enabled)
    return item


def create_double_table_item(value: float, editable: bool = True, enabled: bool = True, decimals: int = 3) \
        -> QtWidgets.QTableWidgetItem:
    return create_string_table_item(f"{value:.{decimals}f}", editable, enabled)


def create_checkbox_table_item(state: bool, enabled: bool = True, tooltip: str = "") -> QtWidgets.QTableWidgetItem:
    item = QtWidgets.QTableWidgetItem()
    item.setToolTip(tooltip)
    if enabled:
        item.setFlags(QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled)
    else:
        item.setFlags(item.flags() ^ QtCore.Qt.ItemIsEnabled)
    if state:
        item.setCheckState(QtCore.Qt.Checked)
    else:
        item.setCheckState(QtCore.Qt.Unchecked)
    return item


def addComboToTable(table,row,options,col=1):
    combo=QtWidgets.QComboBox()
    combo.addItems(options)
    table.setCellWidget(row,col,combo)
    return combo


def addDoubleToTable(table, value, row, col=1, minimum=0.0):
    number_widget = QtWidgets.QLineEdit(str(value))
    validator = LineEditDoubleValidator(number_widget, float(value))
    validator.setBottom(minimum)
    validator.setTop(sys.float_info.max)
    validator.setDecimals(3)
    number_widget.setValidator(validator)
    table.setCellWidget(row, col, number_widget)
    return number_widget, validator


def addCheckBoxToTable(table,state,row,col=1):
    item = create_checkbox_table_item(state)
    table.setItem(row, col, item)
    return item


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
    if QtCore.QSysInfo.productType() != "windows":
        return
    WINDOWS_10 = "10"
    if (QtCore.QSysInfo.productVersion() == WINDOWS_10):
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
