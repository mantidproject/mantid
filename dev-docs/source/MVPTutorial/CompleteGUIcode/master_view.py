from __future__ import (absolute_import, division, print_function)
from qtpy import QtWidgets, QtCore, QtGui

import view
import plot_view


class MasterView(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(MasterView, self).__init__(parent)

        grid = QtWidgets.QVBoxLayout(self)
        self.plot_view = plot_view.PlotView()
        self.options_view = view.View()

        grid.addWidget(self.plot_view)
        grid.addWidget(self.options_view)

        self.setLayout(grid)

    def getOptionView(self):
        return self.options_view

    def getPlotView(self):
        return self.plot_view
