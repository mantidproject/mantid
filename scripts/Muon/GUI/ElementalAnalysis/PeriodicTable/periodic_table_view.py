from __future__ import absolute_import

from PyQt4 import QtCore, QtGui

from silx.gui.widgets import PeriodicTable


class PeriodicTableView(QtGui.QWidget):
    sig_table_changed = QtCore.pyqtSignal(list)
    sig_table_clicked = QtCore.pyqtSignal(
        PeriodicTable.ColoredPeriodicTableItem)

    def __init__(self, parent=None):
        super(PeriodicTableView, self).__init__(parent)

        self.ptable = PeriodicTable.PeriodicTable(self, selectable=True)
        self.ptable.sigElementClicked.connect(self.table_clicked)
        self.ptable.sigSelectionChanged.connect(self.table_changed)

        self.grid = QtGui.QGridLayout()
        self.grid.addWidget(self.ptable)
        self.setLayout(self.grid)

    def table_clicked(self, item):
        self.sig_table_clicked.emit(item)

    def table_changed(self, items):
        self.sig_table_changed.emit(items)
