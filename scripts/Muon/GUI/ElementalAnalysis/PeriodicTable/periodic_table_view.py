# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import

from qtpy import QtWidgets

from Muon.GUI.ElementalAnalysis.PeriodicTable import periodic_table


class PeriodicTableView(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(PeriodicTableView, self).__init__(parent)
        self.ptable = periodic_table.PeriodicTable(self, selectable=True)

        self.grid = QtWidgets.QGridLayout()
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
