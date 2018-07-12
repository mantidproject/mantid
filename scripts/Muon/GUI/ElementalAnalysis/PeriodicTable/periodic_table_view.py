from __future__ import absolute_import

from PyQt4 import QtCore, QtGui

from Muon.GUI.ElementalAnalysis.PeriodicTable import periodic_table


class PeriodicTableView(QtGui.QWidget):
    sig_table_changed = QtCore.pyqtSignal(object)
    sig_table_clicked = QtCore.pyqtSignal(object)

    def __init__(self, parent=None):
        super(PeriodicTableView, self).__init__(parent)

        self.ptable = periodic_table.PeriodicTable(self, selectable=True)
        self.ptable.sigElementClicked.connect(self.table_clicked)
        self.ptable.sigSelectionChanged.connect(self.table_changed)

        self.grid = QtGui.QGridLayout()
        self.grid.addWidget(self.ptable)
        self.setLayout(self.grid)

    def table_clicked(self, item):
        self.sig_table_clicked.emit(item)

    def table_changed(self, items):
        self.sig_table_changed.emit(items)
