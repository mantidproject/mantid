# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=C0103,R0904
# N(DAV)TableWidget
from __future__ import (absolute_import, division, print_function)
from qtpy import QtWidgets, QtCore, QtGui
from qtpy.QtCore import Qt, Signal


class SequentialTableWidget(QtWidgets.QTableWidget):

    keyUpDownPressed = Signal()
    keyEnterPressed = Signal()

    def __init__(self, parent):
        super(SequentialTableWidget, self).__init__(parent)
        self.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding)

    def keyReleaseEvent(self, event):
        if event.isAutoRepeat(): # Ignore somebody holding down keys
            return
        if event.key() in (Qt.Key_Up, Qt.Key_Down):
            self.keyUpDownPressed.emit()
        elif event.key() == Qt.Key_Return:
            self.keyEnterPressed.emit()

    def set_slot_key_up_down_pressed(self, slot):
        self.keyUpDownPressed.connect(slot)

    def set_slot_key_enter_pressed(self, slot):
        self.keyEnterPressed.connect(slot)
