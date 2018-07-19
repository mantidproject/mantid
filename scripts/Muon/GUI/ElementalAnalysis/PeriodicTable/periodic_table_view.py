from __future__ import absolute_import

from PyQt4 import QtGui

from Muon.GUI.ElementalAnalysis.PeriodicTable import periodic_table


class PeriodicTableView(QtGui.QWidget):
    def __init__(self, parent=None):
        super(PeriodicTableView, self).__init__(parent)
        self.ptable = periodic_table.PeriodicTable(self, selectable=True)

        self.grid = QtGui.QGridLayout()
        self.grid.addWidget(self.ptable)
        self.setLayout(self.grid)

    def on_table_lclicked(self, slot):
        self.ptable.sigElementLeftClicked.connect(slot)

    def unreg_on_table_lclicked(self, slot):
        self.ptable.sigElementLeftClicked.disconnect(slot)

    def on_table_rclicked(self, slot):
        self.ptable.sigElementRightClicked.connect(slot)

    def unreg_on_table_rclicked(self, slot):
        self.ptable.sigElementRightClicked.disconnect(slot)

    def on_table_changed(self, slot):
        self.ptable.sigSelectionChanged.connect(slot)

    def unreg_on_table_changed(self, slot):
        self.ptable.sigSelectionChanged.disconnect(slot)
