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
    focusOut = Signal()

    def __init__(self, parent):
        super(SequentialTableWidget, self).__init__(parent)
        self.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding)
        self.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.setFocusPolicy(Qt.StrongFocus)

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

    def focusOutEvent(self, event):
        # not sure this is the best solution
        pass
        #self.clearSelection()
        # self.focusOut.emit()

    def set_selection_to_last_row(self):
        self.setCurrentIndex(self.model().index(self.model().rowCount()-1, 0))
        self.cellClicked.emit(self.model().rowCount()-1, 0)

    def set_slot_key_up_down_pressed(self, slot):
        self.keyUpDownPressed.connect(slot)

    def set_slot_key_enter_pressed(self, slot):
        self.keyEnterPressed.connect(slot)

    def set_slot_for_focus_out_event(self, slot):
        self.focusOut.connect(slot)
