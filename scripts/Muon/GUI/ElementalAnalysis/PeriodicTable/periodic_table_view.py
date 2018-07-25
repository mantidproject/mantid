from __future__ import absolute_import

from PyQt4 import QtCore, QtGui

from Muon.GUI.ElementalAnalysis.PeriodicTable import periodic_table


class PeriodicTableView(QtGui.QWidget):
    sig_table_changed = QtCore.pyqtSignal(object)
    sig_table_lclicked = QtCore.pyqtSignal(object)
    sig_table_rclicked = QtCore.pyqtSignal(object)

    def __init__(self, parent=None):
        super(PeriodicTableView, self).__init__(parent)

        self.ptable = periodic_table.PeriodicTable(self, selectable=True)
        self.ptable.sigElementLeftClicked.connect(self.table_left_clicked)
        self.ptable.sigElementRightClicked.connect(self.table_right_clicked)
        self.ptable.sigSelectionChanged.connect(self.table_changed)

        self.grid = QtGui.QGridLayout()
        self.grid.addWidget(self.ptable)
        self.setLayout(self.grid)

    def table_left_clicked(self, item):
        self.sig_table_lclicked.emit(item)

    def table_right_clicked(self, item):
        self.sig_table_rclicked.emit(item)

    def table_changed(self, items):
        self.sig_table_changed.emit(items)
