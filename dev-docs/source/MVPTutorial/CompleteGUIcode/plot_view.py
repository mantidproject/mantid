from __future__ import (absolute_import, division, print_function)
from qtpy import QtWidgets, QtCore, QtGui
import matplotlib.pyplot as plt

from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas


class PlotView(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(PlotView, self).__init__(parent)

        self.figure = plt.figure()
        grid = QtWidgets.QVBoxLayout(self)
        self.draw()
        self.canvas = self.getWidget()
        grid.addWidget(self.canvas)
        self.setLayout(grid)

    def draw(self):
        ax = self.figure.add_subplot(111)
        ax.clear()
        ax.set_xlim([0.0, 10.5])
        ax.set_ylim([-1.05, 1.05])
        ax.set_xlabel("time ($s$)")
        ax.set_ylabel("$f(t)$")
        return ax

    def getWidget(self):
        return FigureCanvas(self.figure)

    def addData(self, xvalues, yvalues, grid_lines, colour, marker):
        ax = self.draw()
        ax.grid(grid_lines)
        ax.plot(xvalues, yvalues, color=colour, marker=marker, linestyle="--")
        self.canvas.draw()
