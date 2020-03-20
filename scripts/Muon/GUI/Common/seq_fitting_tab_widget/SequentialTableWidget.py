# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=C0103,R0904
# N(DAV)TableWidget
from __future__ import (absolute_import, division, print_function)
from qtpy import QtWidgets
from qtpy.QtCore import Qt, Signal


class TableParameterWidget(QtWidgets.QTableWidgetItem):
    def __init__(self, parameter_value):
        parameter_text = self.convert_parameter_to_string(parameter_value)
        super(TableParameterWidget, self).__init__(parameter_text)

    def convert_parameter_to_string(self, parameter_value):
        parameter = "{:.6g}".format(parameter_value)
        return parameter

    def setValue(self, parameter_value):
        parameter_text = self.convert_parameter_to_string(parameter_value)
        self.setText(parameter_text)


class SequentialTableWidget(QtWidgets.QTableWidget):

    keyUpDownPressed = Signal()
    keyEnterPressed = Signal()

    def __init__(self, parent):
        super(SequentialTableWidget, self).__init__(parent)
        self.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding)
        self.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.setFocusPolicy(Qt.StrongFocus)
        self.itemChanged.connect(self.item_changed)

    # Only accept the key press event if we are not blocking signals e.g due to a fit
    def keyPressEvent(self, event):
        if not self.signalsBlocked():
            super(SequentialTableWidget, self).keyPressEvent(event)

    def keyReleaseEvent(self, event):
        if self.signalsBlocked():
            event.ignore()

        if event.isAutoRepeat():  # Ignore somebody holding down keys
            return
        if event.key() in (Qt.Key_Up, Qt.Key_Down):
            self.keyUpDownPressed.emit()
        elif event.key() == Qt.Key_Return:
            self.keyEnterPressed.emit()

    def set_selection_to_last_row(self):
        self.setCurrentIndex(self.model().index(self.model().rowCount() - 1, 0))
        self.cellClicked.emit(self.model().rowCount() - 1, 0)

    def set_slot_key_up_down_pressed(self, slot):
        self.keyUpDownPressed.connect(slot)

    def set_slot_key_enter_pressed(self, slot):
        self.keyEnterPressed.connect(slot)

    def item_changed(self, item):
        text = item.text()
        if self.test_text_is_value(text):
            item.setText(text)
        else:
            item.setText(str(0))

    @staticmethod
    def test_text_is_value(text):
        try:
            float(text)
            return True
        except ValueError:
            return False
