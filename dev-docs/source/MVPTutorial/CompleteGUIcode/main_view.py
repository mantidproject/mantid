# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtWidgets import QVBoxLayout, QWidget
from typing import Union

from view import View
from plot_view import PlotView


class MainView(QWidget):

    def __init__(self, parent: Union[QWidget, None] = None):
        super().__init__(parent)

        grid = QVBoxLayout(self)
        self._plot_view = PlotView()
        self._options_view = View()

        grid.addWidget(self._plot_view)
        grid.addWidget(self._options_view)

        self.setLayout(grid)

    def get_options_view(self) -> QWidget:
        return self._options_view

    def get_plot_view(self) -> QWidget:
        return self._plot_view
