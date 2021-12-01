# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets
from qtpy.QtCore import Qt, Signal
from mantidqtinterfaces.Muon.GUI.Common.seq_fitting_tab_widget.QSequentialTableModel import FIT_STATUS_COLUMN
from mantidqtinterfaces.Muon.GUI.Common.seq_fitting_tab_widget.SequentialTableDelegates import FitQualityDelegate


class QSelectionTableView(QtWidgets.QTableView):
    keyUpDownPressed = Signal()

    def __init__(self, parent):
        super(QSelectionTableView, self).__init__(parent)
        self.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        self.setAlternatingRowColors(True)

    def resizeColumnsToContents(self):
        if self.model() is None:
            return
        for col in range(self.model().columnCount()):
            if col == 0:
                self.horizontalHeader().setSectionResizeMode(col, QtWidgets.QHeaderView.Stretch)
            else:
                self.horizontalHeader().setSectionResizeMode(col, QtWidgets.QHeaderView.ResizeToContents)

        super(QSelectionTableView, self).resizeColumnsToContents()

    def set_selection_to_last_row(self):
        index = self.model().index(self.model().rowCount() - 1, 0)
        self.setCurrentIndex(index)
        self.clicked.emit(index)

    def keyPressEvent(self, event):
        if not self.signalsBlocked():
            super(QSelectionTableView, self).keyPressEvent(event)

    def keyReleaseEvent(self, event):
        if self.signalsBlocked():
            event.ignore()
        if event.isAutoRepeat():  # Ignore somebody holding down keys
            return
        if event.key() in (Qt.Key_Up, Qt.Key_Down):
            self.keyUpDownPressed.emit()


class QSequentialTableView(QSelectionTableView):
    keyUpDownPressed = Signal()

    def __init__(self, parent):
        super(QSequentialTableView, self).__init__(parent)
        self.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.setItemDelegateForColumn(FIT_STATUS_COLUMN, FitQualityDelegate(self))
        self.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        self.setAlternatingRowColors(True)
