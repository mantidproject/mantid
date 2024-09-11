# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from numpy import ndarray
from qtpy.QtWidgets import QVBoxLayout, QWidget
from typing import Union

import matplotlib.pyplot as plt
from matplotlib.axes import Axes
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas


class PlotView(QWidget):

    def __init__(self, parent: Union[QWidget, None] = None):
        super().__init__(parent)

        self._figure = plt.figure()
        grid = QVBoxLayout(self)
        self.draw()
        self._canvas = FigureCanvas(self._figure)
        grid.addWidget(self._canvas)
        self.setLayout(grid)

    def draw(self) -> Axes:
        ax = self._figure.add_subplot(111)
        ax.clear()
        ax.set_xlim([0.0, 10.5])
        ax.set_ylim([-1.05, 1.05])
        ax.set_xlabel("time ($s$)")
        ax.set_ylabel("$f(t)$")
        return ax

    def addData(self, xvalues: ndarray, yvalues: ndarray, visible: bool, colour: str, marker: str) -> None:
        ax = self.draw()
        ax.grid(visible)
        ax.plot(xvalues, yvalues, color=colour, marker=marker, linestyle="--")
        self._canvas.draw()
